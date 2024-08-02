#include "font_object_transformer.h"

#define RLEFONT_FORMAT_VERSION 4

namespace mcufont
{
    void FontObjectTransformer::encode_dictionary(std::unique_ptr<mf_rlefont_ext_s> &rlefont, const rlefont::encoded_font_t &encoded)
    {
        std::vector<uint16_t> offsets;
        std::vector<uint8_t> data;
        for (const rlefont::encoded_font_t::rlestring_t &r : encoded.rle_dictionary)
        {
            offsets.push_back(data.size());
            data.insert(data.end(), r.begin(), r.end());
        }

        for (const rlefont::encoded_font_t::refstring_t &r : encoded.ref_dictionary)
        {
            offsets.push_back(data.size());
            data.insert(data.end(), r.begin(), r.end());
        }
        offsets.push_back(data.size());

        rlefont->dictionary_data = data;
        rlefont->dictionary_offsets = offsets;
    }

    void FontObjectTransformer::encode_character_range(mf_rlefont_char_range_ext_s &char_range, const DataFile &datafile, const rlefont::encoded_font_t& encoded, 
            const char_range_t& range)
    {
        std::vector<uint16_t> offsets;
        std::vector<uint8_t> data;
        std::map<size_t, unsigned> already_encoded;

        for (int glyph_index : range.glyph_indices)
        {
            if (already_encoded.count(glyph_index))
            {
                offsets.push_back(already_encoded[glyph_index]);
            }
            else
            {
                mcufont::rlefont::encoded_font_t::refstring_t r;
                int width = 0;

                if (glyph_index >= 0)
                {
                    r = encoded.glyphs[glyph_index];
                    width = datafile.GetGlyphEntry(glyph_index).width;
                }

                offsets.push_back(data.size());
                already_encoded[glyph_index] = data.size();

                data.push_back(width);
                data.insert(data.end(), r.begin(), r.end());
            }
        }

        char_range.first_char = range.first_char;
        char_range.char_count = range.char_count;
        char_range.glyph_data = data;
        char_range.glyph_offsets = offsets;
    }

    std::unique_ptr<FontObjectTransformer::mf_rlefont_ext_s> FontObjectTransformer::GetFont(const DataFile &datafile)
    {
        std::unique_ptr<mf_rlefont_ext_s> rlefont(new mf_rlefont_ext_s {
           .version = RLEFONT_FORMAT_VERSION,
        });

        rlefont->font.width = datafile.GetFontInfo().max_width;
        rlefont->font.height = datafile.GetFontInfo().max_height;
        rlefont->font.min_x_advance = mcufont::get_min_x_advance(datafile);
        rlefont->font.max_x_advance = mcufont::get_max_x_advance(datafile);
        rlefont->font.baseline_x = datafile.GetFontInfo().baseline_x;
        rlefont->font.baseline_y = datafile.GetFontInfo().baseline_y;
        rlefont->font.line_height = datafile.GetFontInfo().line_height;
        rlefont->font.flags = datafile.GetFontInfo().flags | mcufont::DataFile::FLAG_BW;
        rlefont->font.fallback_character = mcufont::select_fallback_char(datafile);
        rlefont->font.character_width = &mf_rlefont_character_width;
        rlefont->font.render_character = &mf_rlefont_render_character;

        std::unique_ptr<mcufont::rlefont::encoded_font_t> encoded = rlefont::encode_font(datafile, false);
        encode_dictionary(rlefont, *encoded);
        rlefont->rle_entry_count = encoded->rle_dictionary.size();
        rlefont->dict_entry_count = encoded->ref_dictionary.size();
        
        // Split the characters into ranges
        auto get_glyph_size = [&encoded](size_t i)
        {
            return encoded->glyphs[i].size() + 1; // +1 byte for glyph width
        };
        std::vector<mcufont::char_range_t> ranges = compute_char_ranges(datafile, get_glyph_size, 65536, 16);
        std::vector<mf_rlefont_char_range_ext_s> char_ranges;
        for (const auto &range : ranges)
        {
            mf_rlefont_char_range_ext_s char_range;
            encode_character_range(char_range, datafile, *encoded, range);
            char_ranges.push_back(char_range);
        }
        rlefont->char_ranges = char_ranges;

        return rlefont;
    }
}
