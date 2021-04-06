// Copyright 2013-2021 the openage authors. See copying.md for legal info.

// Warning: this file is a dummy file and was auto-generated by the v0.4.1 converter;
// its purpose is to keep the deprecated gamestate compilable and intact;
// these files keep only the minimum functionality and should not be changed;
// For details, see buildsystem/codegen.cmake and openage/codegen.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include "util/csv.h"



namespace openage {
namespace gamedata {

/**
 * delta definitions for ingame graphics files.
 */
struct graphic_delta {
	int16_t graphic_id;
	int16_t padding_1;
	int32_t sprite_ptr;
	int16_t offset_x;
	int16_t offset_y;
	int16_t display_angle;
	int16_t padding_2;
	static constexpr size_t member_count = 7;
	int fill(const std::string &line);
	bool recurse(const openage::util::CSVCollection &storage, const std::string &basedir);

};

enum class graphics_layer {
    DUMMY
};


/**
 * sound id and delay definition for graphics sounds.
 */
struct sound_prop {
	int16_t sound_delay;
	int16_t sound_id;
	static constexpr size_t member_count = 2;
	int fill(const std::string &line);
	bool recurse(const openage::util::CSVCollection &storage, const std::string &basedir);

};

/**
 * attack sounds for a given graphics file.
 */
struct graphic_attack_sound {
	openage::util::csv_subdata<openage::gamedata::sound_prop> sound_props;
	static constexpr size_t member_count = 1;
	int fill(const std::string &line);
	bool recurse(const openage::util::CSVCollection &storage, const std::string &basedir);

};

/**
 * metadata for ingame graphics files.
 */
struct graphic {
	std::string name;
	std::string filename;
	int32_t slp_id;
	int8_t is_loaded;
	int8_t old_color_flag;
	graphics_layer layer;
	int8_t player_color_force_id;
	int8_t adapt_color;
	uint8_t transparent_selection;
	int16_t sound_id;
	uint16_t frame_count;
	uint16_t angle_count;
	float speed_adjust;
	float frame_rate;
	float replay_delay;
	int8_t sequence_type;
	int16_t graphic_id;
	int8_t mirroring_mode;
	int8_t editor_flag;
	openage::util::csv_subdata<openage::gamedata::graphic_delta> graphic_deltas;
	openage::util::csv_subdata<openage::gamedata::graphic_attack_sound> graphic_attack_sounds;
	static constexpr size_t member_count = 21;
	int fill(const std::string &line);
	bool recurse(const openage::util::CSVCollection &storage, const std::string &basedir);

};

} // gamedata
} // openage
