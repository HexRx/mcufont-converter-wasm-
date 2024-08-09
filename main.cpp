#ifdef __EMSCRIPTEN__
  #include <emscripten.h>
#else
  #define EMSCRIPTEN_KEEPALIVE
#endif
#include "main.h"
#include "mcufont.h"
#include <string.h>
#include <iostream>
#include "freetype_import.hh"
#include "font_object_transformer.h"
#include <random>
#include "optimize_rlefont_ext.h"
#include "export_rlefont.hh"

/* Callback to write to a memory buffer. */
static void pixel_callback(int16_t x, int16_t y, uint8_t count, uint8_t alpha, void *state)
{
    state_t *s = (state_t*)state;
    uint32_t pos;
    int16_t value;

    if (y < 0 || y >= s->height) return;
    if (x < 0 || x + count >= s->width) return;

    while (count--)
    {
        pos = (uint32_t)s->width * y + x;
        value = s->buffer[pos];
        value -= alpha;
        if (value < 0) value = 0;
        s->buffer[pos] = value;

        x++;
    }
}

/* Callback to render characters. */
static uint8_t character_callback(int16_t x, int16_t y, mf_char character, void *state)
{
    state_t *s = (state_t*)state;
    return mf_render_character(s->font, x, y, character, pixel_callback, state);
}

/* Callback to render lines. */
static bool line_callback(const char *line, uint16_t count, void *state)
{
    state_t *s = (state_t*)state;

    mf_render_aligned(s->font, 0, s->y, MF_ALIGN_LEFT, line, count, character_callback, state);
    
    s->y += s->font->line_height;
    return true;
}

/* Callback to just count the lines.
 * Used to decide the image height */
bool count_lines(const char *line, uint16_t count, void *state)
{
    int *linecount = (int*)state;
    (*linecount)++;
    return true;
}

uint32_t height;

EMSCRIPTEN_KEEPALIVE
extern "C" uint8_t *render(const uint8_t *datafile_ptr, const size_t datafile_size, const uint32_t image_width, const char *preview_text)
{
    membuf buf((char*)datafile_ptr, (char*)datafile_ptr + datafile_size);
    std::istream datafile_stream(&buf);
    std::unique_ptr<mcufont::DataFile> datafile = mcufont::DataFile::Load(datafile_stream);
    std::unique_ptr<mcufont::FontObjectTransformer::mf_rlefont_ext_s> rlefont_ext = mcufont::FontObjectTransformer::GetFont(*datafile);

    mf_rlefont_s rlefont = {
        .font = rlefont_ext->font,
        .version = rlefont_ext->version,
        .dictionary_data = rlefont_ext->dictionary_data.data(),
        .dictionary_offsets = rlefont_ext->dictionary_offsets.data(),
        .rle_entry_count = rlefont_ext->rle_entry_count,
        .dict_entry_count = rlefont_ext->dict_entry_count,
        .char_range_count = (uint8_t)rlefont_ext->char_ranges.size(),
    };

    std::vector<mf_rlefont_char_range_s> char_ranges;
    for (const auto &range : rlefont_ext->char_ranges) {
        mf_rlefont_char_range_s char_range;
        char_range.first_char = range.first_char;
        char_range.char_count = range.char_count;
        char_range.glyph_data = range.glyph_data.data();
        char_range.glyph_offsets = range.glyph_offsets.data();
        char_ranges.push_back(char_range);
    }
    rlefont.char_ranges = char_ranges.data();

    const mf_font_s *font = &rlefont.font;

    // Count the number of lines that we need.
    height = 0;
    mf_wordwrap(font, image_width - 2, preview_text, count_lines, &height);
    height *= font->height;
    height += 4;

    // Allocate and clear the image buffer
    state_t state = {
        .buffer = (uint8_t*)malloc(image_width * height),
        .width = image_width,
        .height = height,
        .y = 0,
        .font = font,
    };

    // Initialize image to white
    memset(state.buffer, 255, image_width * height);

    // Render the text
    mf_wordwrap(font, image_width - 2, preview_text, line_callback, &state);

    return state.buffer;
}

EMSCRIPTEN_KEEPALIVE
extern "C" int get_result_height()
{
    return height;
}

std::string datafile_s;

EMSCRIPTEN_KEEPALIVE
extern "C" array_result *import_ttf(const uint8_t *font_data, const size_t font_data_size, const uint32_t out_font_size, const bool monochrome)
{
    membuf sbuf((char*)font_data, (char*)font_data + font_data_size);
    std::istream font_buf(&sbuf);
    std::unique_ptr<mcufont::DataFile> datafile = mcufont::LoadFreetype(font_buf, out_font_size, monochrome);

    mcufont::rlefont::init_dictionary(*datafile);
    std::cout << __FUNCTION__ << ":Done: " << datafile->GetGlyphCount() << " unique glyphs." << std::endl;

    std::stringstream buf;
    datafile->Save(buf);
    buf.seekg(0, std::ios::end);
    int size = buf.tellg();

    datafile_s = buf.str();
    array_result *result = new array_result {
        .size = (uint32_t)size,
        .data_ptr = (void*)datafile_s.c_str()
    };

    return result;
}

void optimize_ext(mcufont::DataFile &datafile, size_t iterations = 50)
{
    bool verbose = false;
    mcufont::rlefont::rnd_t rnd(datafile.GetSeed());

    mcufont::rlefont::update_scores(datafile, verbose);

    size_t size = mcufont::rlefont::get_encoded_size(datafile);

    for (size_t i = 0; i < iterations; i++)
    {
        mcufont::rlefont::optimize_pass(datafile, size, rnd, verbose);
    }

    std::uniform_int_distribution<size_t> dist(0, std::numeric_limits<uint32_t>::max());
    datafile.SetSeed(dist(rnd));
}

EMSCRIPTEN_KEEPALIVE
extern "C" void optimize(const uint8_t *datafile_ptr, const size_t datafile_size, int limit = 100)
{
    membuf buf((char*)datafile_ptr, (char*)datafile_ptr + datafile_size);
    std::istream datafile_stream(&buf);
    std::unique_ptr<mcufont::DataFile> datafile = mcufont::DataFile::Load(datafile_stream);
    size_t oldsize = mcufont::rlefont::get_encoded_size(*datafile);

    std::cout << __FUNCTION__ << ":Original size is " << oldsize << " bytes" << std::endl;

    int i = 0;
    time_t oldtime = time(NULL);
    while (!limit || i < limit)
    {
        optimize_ext(*datafile);

        size_t newsize = mcufont::rlefont::get_encoded_size(*datafile);
        time_t newtime = time(NULL);

        int bytes_per_min = (oldsize - newsize) * 60 / (newtime - oldtime + 1);

        i++;
        std::cout << __FUNCTION__ << ":iteration " << i << ", size " << newsize
                << " bytes, speed " << bytes_per_min << " B/min"
                << std::endl;
    }
}

std::string c_file;

EMSCRIPTEN_KEEPALIVE
extern "C" array_result *export_to_c_code(const uint8_t *datafile_ptr, const size_t datafile_size, const char *font_name)
{
    membuf data_buf((char*)datafile_ptr, (char*)datafile_ptr + datafile_size);
    std::istream datafile_stream(&data_buf);
    std::unique_ptr<mcufont::DataFile> datafile = mcufont::DataFile::Load(datafile_stream);

    std::stringstream buf;
    mcufont::rlefont::write_source(buf, font_name, *datafile);
    buf.seekg(0, std::ios::end);
    int size = buf.tellg();

    c_file = buf.str();
    array_result *result = new array_result {
        .size = (uint32_t)size,
        .data_ptr = (void*)c_file.c_str()
    };

    return result;
}
