#ifndef BRUNGART_DEVICE_H
#define BRUNGART_DEVICE_H

#include <vector>
#include <fstream>

#include "EPICLib/Device_base.h"
#include "EPICLib/Symbol.h"
#include "EPICLib/Geometry.h"

#include "Response_object.h"
#include "Message.h"
#include "CRM_utterance_stats.h"

namespace GU = Geometry_Utilities;
#

/*
A Brungart_device has some number of sources, each at a location, speaking a message
with a particular format consisting of:

Ready <call sign>  go to <color> <number> now.

A particular call sign identifies the target source. The user responds by pointing to 
the corresponding color-number response shown as colored digits on the display with a mouse.
The loudness of the source is another parameter.


Bolia et al (2000) corpus of 2048 unique sentences
8 call signs are arrow, barn, charlie, eagle, hopper, laker, ringo, and tiger
4 colors are blue, green, red, white
8 numbers are one, two, ... eight.
8 speakers, four male, four female speakerstotal of 

Brungart & Simpson 2005.
Exp. 1. Three spatial configurations, two sources, but same speaker

Exp. 2. Seven spatial configurations, including one where all talkers have the same location.
In each block, each of seven speakers assign to a location. Four male, 
3 female to sound like male. First target talker selected at random, 
then 25% change that differnt talker would be the target talker on the next trial. 
Target talker started 100 ms before others.

*/

namespace GU = Geometry_Utilities;

// see implementation file for conversion functions and constants 
// pertaining to screen layout

class Brungart_device : public Device_base {
public:
	Brungart_device(const std::string& id, Output_tee& ot);
			
	virtual void initialize();
	virtual void set_parameter_string(const std::string&);
	virtual std::string get_parameter_string() const;
//	virtual void display() const;	// output is always to device_out member
//	virtual std::string processor_info() const;	// override default to produce string containing output info

	virtual void handle_Start_event();
	virtual void handle_Stop_event();
	virtual void handle_Delay_event(const Symbol& type, const Symbol& datum, 
		const Symbol& object_name, const Symbol& property_name, const Symbol& property_value);
	virtual void handle_Keystroke_event(const Symbol& key_name);
	virtual void handle_Ply_event(const Symbol& cursor_name, const Symbol& target_name,
		GU::Point new_location, GU::Polar_vector movement_vector);
		
private:

	struct Speaker {
		Speaker(const Symbol& id_, const Symbol& gender_, double pitch_mean_, double pitch_sd_,
			double loudness_mean_, double loudness_sd_) :
			id(id_), gender(gender_), pitch_mean(pitch_mean_), pitch_sd(pitch_sd_), 
			loudness_mean(loudness_mean_), loudness_sd(loudness_sd_) {}
		Symbol id;
		Symbol gender;
		double pitch_mean;
		double pitch_sd;
		double loudness_mean;
		double loudness_sd;
	};



	enum State_e {START, PRESENT_CURSOR, START_TRIAL, PRESENT_STIMULUS, 
		NEXT_WORD, ENABLE_RESPONSE, WAITING_FOR_RESPONSE, 
		SHUTDOWN};
	
	State_e state;
	
	// parameters
	std::string condition_string;
	int n_trials;
	int n_speakers;
	static const int n_speakers_max_c = 4;
	static const int n_speaker_conditions_c = 3;
	
	long probe_targets_delay;

	// stimulus generation	
	Words_t callsigns;
	Words_t colors;
	Words_t digits;
	std::vector<Speaker> speakers; // speaker parameters
	// these dimensions are fixed in the CRM corpus
	CRM_utterance_stats utterance_data[8][8][4][8];
	
	// the following are n_speakers in length; first cell is for target, rest for maskers
	std::vector<Symbol> stream_names;
	std::vector<double> loudnesses;
	// 3 cells, one for each speaker condition
	std::vector<std::string> org_masking_condition_labels;
	std::vector<std::string> masking_condition_labels;
	// outer vector is 3 cells, one for each speaker condition
	// inner vector is 4 cells, one for each stream
//	typedef std::vector<std::vector<Symbol> > Condition_spec_t;

//	Condition_spec_t speaker_genders;
//  Condition_spec_t speaker_ids;

	std::vector<double> org_target_snrs;
	std::vector<double> rep_target_snrs;
	std::vector<double> target_snrs;
	
	std::vector<Message> messages;

	// following are specs for current target
	Symbol target_callsign;
	Symbol target_color;
	Symbol target_digit;
	double target_loudness;
			
	int word_counter;

	// display constants
	double response_object_dimensions;
	GU::Size response_object_size;

	// display state
	GU::Point cursor_location;
	Symbol current_pointed_to_object;

	typedef std::vector<Response_object> Response_objects_t;
	Response_objects_t response_objects;
					
	// data accumulation
	std::ofstream output_file;		
	int trial;
	int snr_index;
	int condition_index;
	long rt;
	int n_completely_correct;
	int n_color_only_correct;
	int n_digit_only_correct;
	int n_completely_incorrect;
	int n_masker_both;
	int n_masker_color_only;
	int n_masker_digit_only;
	int n_masker_neither;
	
	int n_target_color;
	int n_masker_color;
	int n_neither_color;
	int n_target_digit;
	int n_masker_digit;
	int n_neither_digit;
	
	int n_color_digit_table[3][3];
	
	long stimulus_onset_time;
    std::string corpus_version_info;
	
	// helpers
	void parse_condition_string();
	void load_utterance_corpus_data();
	void present_number_of_speakers();
	void remove_number_of_speakers();
	void present_cursor();
	void signal_trial_start();
	void reset_for_run();
	void setup_first_run();
	bool setup_next_run();
	void set_parameter(const std::string& proc_name, const std::string& param_name, const std::string& spec, double value);
	
	void create_messages();
	void present_stimulus();
	void present_next_word();
	void present_word(const Symbol& source, char stem, const Symbol& word, const Symbol& speaker_gender, const Symbol& speaker_id, double loudness, long duration);
	void start_masking_noise();
	void stop_masking_noise();
	void present_response_objects();
	void present_response_object(Response_object& response_object);
	void present_none_response_object();
	void remove_response_objects();
	void score_response();
	void output_statistics();

	// rule out default ctor, copy, assignment
	Brungart_device(const Brungart_device&);
	Brungart_device& operator= (const Brungart_device&);
};

#endif


