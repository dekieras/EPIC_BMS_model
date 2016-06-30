/*
 *  Message.cpp
 *  BrungartV2_device
 *
 *  Created by David Kieras on 7/19/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "Message.h"

#include "EPICLib/Device_base.h"
#include "EPICLib/Symbol.h"
#include "EPICLib/Geometry.h"
#include "EPICLib/Speech_word.h"
#include "EPICLib/Numeric_utilities.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace GU = Geometry_Utilities;
using namespace std;


vector<long> Message::durations;
Words_t Message::skeleton;

Message::Message() : device_ptr(0)
{
		initialize();
}

Message::Message(Device_base * device_ptr_, const Symbol& stream_name_, const Symbol& speaker_gender_, const Symbol& speaker_id_, 
        int talker_idx_, int callsign_idx_, int color_idx_, int digit_idx_,
		const CRM_utterance_stats& utterance_stats,
		double baseline_loudness,
		const Symbol& callsign_, const Symbol& color_, const Symbol& digit_) :
	device_ptr(device_ptr_), stream_name(stream_name_), speaker_gender(speaker_gender_), speaker_id(speaker_id_), 
    talker_idx(talker_idx_), callsign_idx(callsign_idx_), color_idx(color_idx_), digit_idx(digit_idx_),
	callsign(callsign_), color(color_), digit(digit_)
{
		initialize();
		message[1] = callsign;
		message[3] = color; // follows 6-beat analysis, with "goto"
		message[4] = digit;
		// below is the per-utterance segment duration
//		long segment_duration = long(utterance_stats.duration * 1000. / 6.); // duration of each segment in ms
		
		// generate loudnesses, pitches, and durations
		for(int i = 0; i < skeleton.size(); i++) {
			// force loudness to be the same value with no speaker/utterance variation
//			loudnesses.push_back(60.0 + baseline_loudness);
			loudnesses.push_back(utterance_stats.loudnesses[i] + baseline_loudness);
			// force pitches to same value with no speaker/utterance variation
//			pitches.push_back(256.0);
			pitches.push_back(utterance_stats.pitches[i]);
//			pitches.push_back(normal_random_variable(mean_pitch, sd_pitch));
//			durations.push_back(segment_duration);
			}
			
		// force correlation of color and digit
		// applied to 6-beat messages
//		loudnesses[4] = loudnesses[3];
//		pitches[4] = pitches[3];
		// force correlation of all three content words
		// experiment with this 5/11/12
		// applied to 6-beat messages
//		loudnesses[3] = loudnesses[1];
//		loudnesses[4] = loudnesses[1];		
//		pitches[3] = pitches[1];
//		pitches[4] = pitches[1];

}

// this version follows Greg's 6-beat analysis by grouping "go" and "to" together into "goto"
void Message::initialize()
{
	if(skeleton.empty()) {
		// a skeleton for the message
		skeleton.push_back(Symbol("ready"));	//0
		skeleton.push_back(Symbol("callsign")); //1
		skeleton.push_back(Symbol("goto"));		//2
		skeleton.push_back(Symbol("color"));	//3
		skeleton.push_back(Symbol("digit"));	//4
		skeleton.push_back(Symbol("now"));		//5
		}
		
	if(durations.empty()) {
			
		// this constant is a value from Greg's corpus statistics of 5/14/2012
		// it is the average utterance duration across the 2048 utterances in the corpus
//		double mean_utterance_duration_c = 1.761337891; // crmstats_v15
		double mean_utterance_duration_c = 1.760583496;	// crmstats_wdseg_v1
		long segment_duration = long(mean_utterance_duration_c * 1000. / 6.); // duration of each segment in ms
		// message word durations
		durations.push_back(segment_duration);
		durations.push_back(segment_duration);
		durations.push_back(segment_duration);
		durations.push_back(segment_duration);
		durations.push_back(segment_duration);
		durations.push_back(segment_duration);
		}
	Assert(skeleton.size() == durations.size());

	message = skeleton;
}

// present the  word
void Message::present_word(int trial, int word_counter)
{
	Assert(word_counter >= 0 && word_counter < skeleton.size());
	// create a name for the word object
	ostringstream oss;
	// trial number is part of name, ensuring a unique name across trials
	oss << stream_name  << "_" << trial << "_" << word_counter;
	Symbol wordname = Symbol(oss.str());
	long duration = durations[word_counter];
	
	Speech_word word;
	word.location = GU::Point(0., 0.);
	word.name = wordname;
	word.stream_name = stream_name;
	word.content = message[word_counter];
	word.speaker_gender = speaker_gender;
	word.speaker_id = speaker_id;
    word.utterance_id = talker_idx * 1000 + callsign_idx * 100 + color_idx * 10 + digit_idx;
	word.pitch = pitches[word_counter];
//	word.loudness = normal_random_variable(mean_loudness, sd_loudness);
	word.loudness = loudnesses[word_counter];
//	word.pitch = normal_random_variable(mean_pitch, sd_pitch);
	word.duration = duration;
	
//	device_ptr->make_auditory_speech_event(wordname, stream_name, message[word_counter], speaker_gender, speaker_id, loudness, duration);
	device_ptr->make_auditory_speech_event(word);
	// if this is the first word, supply the location corresponding to the source
//	if(word_counter == 0)
//		device_ptr->set_auditory_stream_location(stream_name, GU::Point(0., 0.));	
}

long Message::get_duration(int word_counter)
{
	Assert(word_counter >= 0 && word_counter < durations.size());
	return durations[word_counter];
}


