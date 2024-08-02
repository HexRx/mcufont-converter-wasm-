#pragma once

#include "datafile.hh"
#include "mcufont.h"
#include "exporttools.hh"
#include "mf_rlefont.h"
#include "encode_rlefont.hh"

namespace mcufont
{
    class FontObjectTransformer : public DataFile {
    public:
        struct mf_rlefont_char_range_ext_s : public mf_rlefont_char_range_s {
            std::vector<uint16_t> glyph_offsets;
            std::vector<uint8_t> glyph_data;
        };

        struct mf_rlefont_ext_s {
            struct mf_font_s font;
            const uint8_t version;
            std::vector<uint16_t> dictionary_offsets;
            std::vector<uint8_t> dictionary_data;
            uint8_t rle_entry_count;
            uint8_t dict_entry_count;
            std::vector<mf_rlefont_char_range_ext_s> char_ranges;
        };

        static std::unique_ptr<mf_rlefont_ext_s> GetFont(const DataFile &dataFile);
    private:
        static inline void encode_dictionary(std::unique_ptr<mf_rlefont_ext_s> &rlefont, const rlefont::encoded_font_t &encoded);
        static inline void encode_character_range(mf_rlefont_char_range_ext_s &char_range, const DataFile &datafile, const rlefont::encoded_font_t& encoded, 
            const char_range_t& range);
    };
}