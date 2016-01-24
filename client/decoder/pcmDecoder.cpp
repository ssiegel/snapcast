/***
    This file is part of snapcast
    Copyright (C) 2014-2016  Johannes Pohl

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
***/

#include "common/snapException.h"
#include "common/log.h"
#include "pcmDecoder.h"


#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

struct riff_wave_header
{
	uint32_t riff_id;
	uint32_t riff_sz;
	uint32_t wave_id;
};


struct chunk_header
{
	uint32_t id;
	uint32_t sz;
};


struct chunk_fmt
{
	uint16_t audio_format;
	uint16_t num_channels;
	uint32_t sample_rate;
	uint32_t byte_rate;
	uint16_t block_align;
	uint16_t bits_per_sample;
};


PcmDecoder::PcmDecoder() : Decoder()
{
}


bool PcmDecoder::decode(msg::PcmChunk* chunk)
{
	return true;
}


msg::SampleFormat PcmDecoder::setHeader(msg::Header* chunk)
{
	if (chunk->payloadSize < 44)
		throw SnapException("PCM header too small");

    struct riff_wave_header riff_wave_header;
	struct chunk_header chunk_header;
	struct chunk_fmt chunk_fmt;
	chunk_fmt.sample_rate = 0;

	size_t pos(0);
	memcpy(&riff_wave_header, chunk->payload + pos, sizeof(riff_wave_header));
	pos += sizeof(riff_wave_header);
	if ((riff_wave_header.riff_id != ID_RIFF) || (riff_wave_header.wave_id != ID_WAVE))
		throw SnapException("Not a riff/wave header");

	bool moreChunks(true);
	do
	{
		if (pos + sizeof(chunk_header) > chunk->payloadSize)
			throw SnapException("riff/wave header incomplete");
		memcpy(&chunk_header, chunk->payload + pos, sizeof(chunk_header));
		pos += sizeof(chunk_header);
		switch (chunk_header.id)
		{
		case ID_FMT:
			if (pos + sizeof(chunk_fmt) > chunk->payloadSize)
				throw SnapException("riff/wave header incomplete");
			memcpy(&chunk_fmt, chunk->payload + pos, sizeof(chunk_fmt));
			pos += sizeof(chunk_fmt);
			/// If the format header is larger, skip the rest
			if (chunk_header.sz > sizeof(chunk_fmt))
				pos += (chunk_header.sz - sizeof(chunk_fmt));
			break;
		case ID_DATA:
			/// Stop looking for chunks
			moreChunks = false;
			break;
		default:
			/// Unknown chunk, skip bytes
			pos += chunk_header.sz;
		}
	}
	while (moreChunks);


	if (chunk_fmt.sample_rate == 0)
		throw SnapException("Sample format not found");

	msg::SampleFormat sampleFormat(
		chunk_fmt.sample_rate,
		chunk_fmt.bits_per_sample,
		chunk_fmt.num_channels);

	return sampleFormat;
}




