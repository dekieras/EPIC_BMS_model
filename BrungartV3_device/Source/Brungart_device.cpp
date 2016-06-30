/*
Modify so that when it starts up, it puts up the number of speakers on the screen for a second so that production rules 
can set a strategy switch.

This version implements 1, 2, or 3 masking speakers with the same gender and same/different talker characteristics

For each trial,
	1. Display a response matrix, which is always the same: 4 colored rows of 8 digits.
	1. Sound a beep.
	3. Wait for a delay.
	4. Start the auditory presentation; two message strings, each a separate word, launched simultaneously from each source.
	5. Wait for response, pointing to color-digit object.
	7. record score. RT not recorded.

The 2-4 messages: One always contains the "baron" callsign; the others contain a different callsign selected at random.
The subject is supposed to respond to the baron phrase only. The color-digit are randomly selected without replacement for all of the phrases.

*/


#include "Brungart_device.h"
#include "Response_object.h"
#include "EPICLib/Geometry.h"
#include "EPICLib/Output_tee_globals.h"
#include "EPICLib/Numeric_utilities.h"
#include "EPICLib/Random_utilities.h"
#include "EPICLib/Symbol_utilities.h"
#include "EPICLib/Device_exception.h"
#include "EPICLib/Standard_Symbols.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace GU = Geometry_Utilities;
using namespace std;

// handy internal constants
const Symbol Fixation_point_c("Fixation_point");
const Symbol Display_c("Display");
const Symbol Loudspeaker_c("Loudspeaker");
const Symbol Speaker_n_field_c("Speaker_n_field");

const long iti_c = 6000;	// time between response or time out and next trial start
const long timeout_time_c = 2000;	// time between last word presentation and time-out on waiting for response
const long response_enable_delay_time_c = 500;	// time between last word presentation and enabling responses

const int n_outer_iterations = 6;
const Symbol target_speaker("FS1");
const Symbol target_gender = Female_c;
const int n_inner_iterations = 6;
// const double masker_loudness = 60.; 
const double masker_loudness = 0.; // baseline for loudness - crmstats has 60 as baseline

// using 6-beat segmentation
int message_length_c = 6;



Brungart_device::Brungart_device(const std::string& id, Output_tee& ot) :
		Device_base(id, ot), n_trials(0), n_speakers(2), condition_string("4 2 rep"), 
		//stream_names(n_speakers_max_c), 
		//speaker_genders(n_speaker_conditions_c), speaker_ids(n_speaker_conditions_c),
		org_masking_condition_labels(n_speaker_conditions_c),masking_condition_labels(n_speaker_conditions_c),loudnesses(n_speakers_max_c, masker_loudness),
		messages(n_speakers_max_c),
		trial(0), n_completely_correct(0), n_color_only_correct(0), n_digit_only_correct(0),
		snr_index(0), condition_index(0),
		state(START)
{
	Assert(device_out);

// cannot do this load here because the current working directory may not be set yet.
	load_utterance_corpus_data();
	 
	/* Experiment Constants */
	// rearranged to match CRM corpus conventions
	
	callsigns.push_back("charlie"); // 0
	callsigns.push_back("ringo");	// 1
	callsigns.push_back("laker");	// 2
	callsigns.push_back("hopper");	// 3
	callsigns.push_back("arrow");	// 4
	callsigns.push_back("tiger");	// 5
	callsigns.push_back("eagle");	// 6
	callsigns.push_back("baron");	// 7 target callsign at end

	target_callsign = callsigns[7];
		
	colors.push_back("Blue");	// 0
	colors.push_back("Red");	// 1
	colors.push_back("White");	// 2
	colors.push_back("Green");	// 3

	digits.push_back("1");	// 0
	digits.push_back("2");
	digits.push_back("3");
	digits.push_back("4");
	digits.push_back("5");
	digits.push_back("6");
	digits.push_back("7");
	digits.push_back("8");	// 7
	
	stream_names.push_back("AT");	// target stream name kept at beginning
	stream_names.push_back("BM");	// these are kept in order
	stream_names.push_back("CM");
	stream_names.push_back("DM");

	
/*  from Greg, 2/13/12
			f0 [Hz]			Amp [dB]
		sex	mean	stdev	mean	stddev	sex
Talker 0	121.5	5.87	-31		4.5		m
Talker 1	129.4	9.44	-31		3.4		m
Talker 2	127.7	6.77	-31		4.3		m
Talker 3	132.3	12.25	-31		5.2		m
Talker 4	258.6	22.81	-32		5.5		f
Talker 5	248.1	17.8	-32		5.1		f
Talker 6	252.3	22.07	-32		5.1		f
Talker 7	250.1	13.77	-31		4.3		f
*/
	// speakers[0] through 3 are male, 4 through 7 are female
	speakers.push_back(Speaker("MS1", Male_c,	121.5,  5.87,  -1, 4.5));	// 0
	speakers.push_back(Speaker("MS2", Male_c,	129.4,  9.44,  -1, 3.4));	// 1
	speakers.push_back(Speaker("MS3", Male_c,	127.7,  6.77,  -1, 4.3));	// 2
	speakers.push_back(Speaker("MS4", Male_c,	132.3, 12.25,  -1, 5.2));	// 3
	speakers.push_back(Speaker("FS1", Female_c,	258.6, 22.81,  -2, 5.5));	// 4
	speakers.push_back(Speaker("FS2", Female_c,	248.1, 17.8,   -2, 5.1));	// 5
	speakers.push_back(Speaker("FS3", Female_c,	252.3, 22.07,  -2, 5.1));	// 6
	speakers.push_back(Speaker("FS4", Female_c,	250.1, 13.77,  -2, 4.3));	// 7
	
	// NOTE NOTE NOTE as of 5/2/12 code below does not use baseline per-speaker loudness (the -1 and -2 above).
	
	// must generate these at run time
		org_masking_condition_labels[0] = "TDDD";

		org_masking_condition_labels[1] = "TSSS";
		org_masking_condition_labels[2] = "TTTT";
	
//	const int n_snrs = 10;
//	const int n_snrs = 9;
//	const double snrs[n_snrs] = {-12, -9, -6, -3, 0, +3, +6, +9, +12, +15};
//	const double snrs[n_snrs] = {-12, -9, -6, -3, 0, +3, +6, +9, +12};
//	const double snrs[n_snrs] = {-12, -9, -6, -3, 0, +3, +6, +9, +12};
    // rep is the snr range in the Brungart replication experiment; org is the snr range of the original 2001 experiment
    rep_target_snrs = {-18, -15, -12, -9, -6, -3, 0, 3, 6, 9};
	org_target_snrs = {-12, -9, -6, -3, 0, +3, +6, +9, +12, +15};
//	const double louds[n_speakers_max_c] = {masker_loudness, masker_loudness, masker_loudness, masker_loudness};
//	copy(louds, louds+n_speakers_max_c, back_inserter(loudnesses));
		
	double vert_loc_start = +3.;
	double vert_loc_inc = -2.;
	double hor_loc_start = -7.;
	double hor_loc_inc = +2.;
	// create the container of search objects - always the same on each trial
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 8; j++) {
			// id number is (row (i) + 1) * 10 + col (j + 1)
			response_objects.push_back(Response_object(
				(i+1) * 10 + j+1, 
				GU::Point(hor_loc_start + hor_loc_inc * j, vert_loc_start + vert_loc_inc * i), 
				colors[i],
				digits[j])
				);
			}
					
	parse_condition_string();
	
//	initialize();

}


/* CRM Corpus Codes and Info 

Talkers: 8 talkers, coded 0 - 7; 0 - 3 are male, 4 - 7 female. (4 each)

Callsign: 8 callsigns, coded 0 -7:
0 Charlie
1 Ringo
2 Laker
3 Hopper
4 Arrow 
5 Tiger
6 Eagle
7 Baron // used for target throughout

Color: 4 colors coded 0 - 3; notice White is used
0 Blue
1 Red
2 White
3 Green

Digit: 8 digits, coded 0 - 7, one less than actual digit value
0 1
1 2
2 3
3 4
4 5
5 6
6 7
7 8
*/



void Brungart_device::load_utterance_corpus_data()
{
	ifstream infile("crm_utterance_corpus_data.txt");
	if(!infile)
		throw Device_exception("Could not open crm_utterance_corpus_data.txt!");
    
    // read, output, and discard the first line that has version information
    getline(infile, corpus_version_info);
	if(!infile)
		throw Device_exception("Could not read crm_utterance_corpus_data.txt!");
    device_out << "Corpus data version: " << corpus_version_info << endl;
		
	// these values are fixed in the corpus
	for(int ispkr = 0; ispkr < 8; ispkr++) {
		for (int icallsign = 0; icallsign < 8; icallsign++) {
			for(int icolor = 0; icolor < 4; icolor++) {
				for(int idigit = 0; idigit < 8; idigit++) {
					// read and verify subscripts
					int spk, cal, clr, dig;
					if(!(infile >> spk >> cal >> clr >> dig))
						throw Device_exception("failure to read utterance subscripts");
                   // device_out << spk << ' ' << cal << ' ' << clr << ' ' << dig << endl;
					Assert(ispkr == spk && icallsign == cal && icolor == clr && idigit == dig);
					if(!(infile >> utterance_data[ispkr][icallsign][icolor][idigit]))
						throw Device_exception("failure to read utterance_stats");
						
					if(icallsign == 7 && icolor == 3 && idigit == 7)
						device_out << ispkr << ' ' << icallsign << ' ' << icolor << ' ' << idigit << ' '
							<< utterance_data[ispkr][icallsign][icolor][idigit] << endl;
					}
				}
			}
		}
}

void Brungart_device::parse_condition_string()
{
	// build an error message string in case we need it
	string error_msg(condition_string);
	error_msg += "\n Should be: number of trials followed by number of speakers";
	istringstream iss(condition_string);
	int nt;
	iss >> nt;
	// do all error checks before last read
	if(!iss)
		throw Device_exception(this, string("Incorrect condition string: ") + error_msg);
	if(nt <= 0)
		throw Device_exception(this, string("Number of trials must be positive: ") + error_msg);
	int ns;
	iss >> ns;
	if(!iss)
		throw Device_exception(this, string("Incorrect condition string: ") + error_msg);
	if(ns > n_speakers_max_c || ns < 2)
		throw Device_exception(this, string("Number of speakers must be >= 2, <=4: ") + error_msg);
    string version;
    iss >> version;
	if(!iss)
		throw Device_exception(this, string("Incorrect condition string: ") + error_msg);
    if (version != "rep" && version != "org")
		throw Device_exception(this, string("version must be \"rep\" or \"org\": ") + error_msg);
		
	n_trials = nt;
	n_speakers = ns;
    if(version == "rep")
        target_snrs = rep_target_snrs;
    else if(version == "org")
        target_snrs = org_target_snrs;
}

void Brungart_device::set_parameter_string(const string& condition_string_)
{
	condition_string = condition_string_;
	parse_condition_string();
}

string Brungart_device::get_parameter_string() const
{
	return condition_string;
}



void Brungart_device::initialize()
{
	state = START;
	reset_for_run();
	// start middle loop
	condition_index = 0;
	// start inner loop
	snr_index = 0;
    
//    load_utterance_corpus_data(); // why were we doing this here?

}

void Brungart_device::reset_for_run()
{
	// replications loop
	trial= 0;
	n_completely_correct = 0;
	n_color_only_correct = 0;
	n_digit_only_correct = 0;
	n_completely_incorrect = 0;
	n_masker_both = 0;
	n_masker_color_only = 0;
	n_masker_digit_only = 0;
	n_masker_neither = 0;
	n_target_color = 0;
	n_masker_color = 0;
	n_neither_color = 0;
	n_target_digit = 0;
	n_masker_digit = 0;
	n_neither_digit = 0;
	for(int i = 0; i < 3; i++)
		for(int j = 0; j < 3; j++)
			n_color_digit_table[i][j] = 0;

}

void Brungart_device::handle_Start_event()
{
//	device_out << processor_info() << "received Start_event" << endl;
	schedule_delay_event(500);
}

void Brungart_device::handle_Stop_event()
{
	device_out << processor_info() << "received Stop_event" << endl;
//	output_statistics();
		
}

void Brungart_device::setup_first_run()
{
	snr_index = 0;
	condition_index = 0;
	target_loudness = target_snrs[snr_index] + masker_loudness;
	loudnesses[0] = target_loudness;
	// set masking condition lables to reflect number of speakers
	for(int i = 0; i < n_speaker_conditions_c; i++)
		masking_condition_labels[i] = org_masking_condition_labels[i].substr(0, n_speakers);
	reset_for_run();
	output_file.open("Brungart_device_output.txt");
    output_file << corpus_version_info << endl;
    output_file << get_human_prs_filename() << endl;
}

bool Brungart_device::setup_next_run()
{
	snr_index++;
	if(snr_index == target_snrs.size()) {
		snr_index = 0;
		condition_index++;
		if(condition_index == n_speaker_conditions_c) {
			output_file.close();
			return true;	// time to stop
			}
		output_file << endl;	// a blank line
		}
	target_loudness = target_snrs[snr_index] + masker_loudness;
	loudnesses[0] = target_loudness;
	reset_for_run();
	return false;	// do the next run

}


void Brungart_device::handle_Delay_event(const Symbol&, const Symbol&, 
		const Symbol&, const Symbol&, const Symbol&)
{	
	switch(state) {
		case START:
			present_number_of_speakers();
			setup_first_run();		// can't do this during construction, because connection to human for parameter setting not yet made
			state = PRESENT_CURSOR;
			schedule_delay_event(300);
			break;
		case PRESENT_CURSOR:
			remove_number_of_speakers();
			present_cursor();
			state = START_TRIAL;
			schedule_delay_event(200);
			break;
		case START_TRIAL:
			signal_trial_start();
			// make trial start time fluctuate
			schedule_delay_event(450 + random_int(100));
			state = PRESENT_STIMULUS;
			break;
		case PRESENT_STIMULUS: 
			present_stimulus();
			break;
		case NEXT_WORD: 
			present_next_word();
			break;
		case ENABLE_RESPONSE: 
			present_response_objects();
			state = WAITING_FOR_RESPONSE;
			break;
		case SHUTDOWN:
			stop_simulation();
			break;
		case WAITING_FOR_RESPONSE:
		default:
			throw Device_exception(this, "Device delay event in unknown or improper device state");
			break;
		}
}

void Brungart_device::present_number_of_speakers()
{
	make_visual_object_appear(Speaker_n_field_c, GU::Point(0., 0.), GU::Size(1., 1.));
	set_visual_object_property(Speaker_n_field_c, Color_c, White_c);
	set_visual_object_property(Speaker_n_field_c, Shape_c, Rectangle_c);
	set_visual_object_property(Speaker_n_field_c, Text_c, Symbol(n_speakers));
}

void Brungart_device::remove_number_of_speakers()
{
	make_visual_object_disappear(Speaker_n_field_c);
}

void Brungart_device::present_cursor()
{
	make_visual_object_appear(Cursor_name_c, cursor_location, GU::Size(2., 2.));
	set_visual_object_property(Cursor_name_c, Color_c, Black_c);
	set_visual_object_property(Cursor_name_c, Shape_c, Cursor_Arrow_c);
}

void Brungart_device::signal_trial_start()
{
//	make_visual_object_appear(Fixation_point_c, probe_location, GU::Size(1., 1.));
//	set_visual_object_property(Fixation_point_c, Color_c, Red_c);
//	set_visual_object_property(Fixation_point_c, Shape_c, Cross_Hairs_c);
	// testing - not part of the experiment
	// destroy the beep_source_name loudspeaker source if created ...
/*	if(beep_source_name != Symbol())	// source name was not assigned
		destroy_auditory_source(beep_source_name);
	
	// create a source with a name and location different for each trial
	beep_source_name = concatenate_to_Symbol(Loudspeaker_c, trial);
	create_auditory_source(beep_source_name, GU::Point(0, -3 - (trial/10.)));
	// give the sound object a unique name
	beep_name = concatenate_to_Symbol(Symbol("Beep"), trial);
	make_auditory_sound_start(beep_name, beep_source_name, Symbol("Beep-timbre"), 50);
	set_auditory_sound_property(beep_name, Pitch_c, Symbol(440));
*/
}

// colors and digits need to be non-repeated across the 2-4 messages
// first [0] message is the target, rest are maskers
// create all four, even if only two will be used
// speaker genders and id
void Brungart_device::create_messages()
{
	// generate randomization of masker callsigns, target and masker colors, target and masker digits
	// target callsign is fixed at [7], so randomize 0-6 for maskers
	int callsign_indices[8] = {7, 0, 1, 2, 3, 4, 5, 6};
//	random_shuffle(callsign_indices+1, callsign_indices+8);
	shuffle(callsign_indices+1, callsign_indices+8, get_Random_engine());
	int color_indices[4] = {0, 1, 2, 3};
//	random_shuffle(color_indices, color_indices+4);
	shuffle(color_indices, color_indices+4, get_Random_engine());
	int digit_indices[8] = {0, 1, 2, 3, 4, 5, 6, 7};
//	random_shuffle(digit_indices, digit_indices+8);
	shuffle(digit_indices, digit_indices+8, get_Random_engine());
	
	Assert(target_callsign == callsigns[callsign_indices[0]]);
	target_color = colors[color_indices[0]];
	target_digit = digits[digit_indices[0]];
	
	vector<int> message_speakers; // first is always target, always 3 maskers following
	
	for(int i = 0; i < n_speakers; i++) {
		// choose gender, speaker of target, then choose maskers depending on condition
		// these arrays contain the indicies into the speakers array
		int male_speakers[4] = {0, 1, 2, 3};
		int female_speakers[4] = {4, 5, 6, 7};
//		random_shuffle(male_speakers, male_speakers+4);
		shuffle(male_speakers, male_speakers+4, get_Random_engine());
//		random_shuffle(female_speakers, female_speakers+4);
		shuffle(female_speakers, female_speakers+4, get_Random_engine());
		int target_gender = random_int(2);  // 0 for male, 1 for female
	
		switch (condition_index) {
			case 0: { //TD different genders and speakers
				if(target_gender == 0) {// male
					message_speakers.push_back(male_speakers[0]); // first in random shuffled
					copy(female_speakers, female_speakers+3, back_inserter(message_speakers)); // first three in shuffled
					}
				else {// female
					message_speakers.push_back(female_speakers[0]);
					copy(male_speakers, male_speakers+3, back_inserter(message_speakers));
					}
				break;
				}
			case 1: { //TS - same gender but different talkers
				if(target_gender == 0) { // male
					message_speakers.push_back(male_speakers[0]); // first in random shuffled
					copy(male_speakers+1, male_speakers+4, back_inserter(message_speakers)); // remaining three in shuffled
					}
				else {// female
					message_speakers.push_back(female_speakers[0]);
					copy(female_speakers+1, female_speakers+4, back_inserter(message_speakers));
					}
				break;
				}
			case 2: { //TT - same talker of that gender
				if(target_gender == 0) { // male
					message_speakers.push_back(male_speakers[0]); // first in random shuffled
					for(int i = 1; i != 4; i++)
						message_speakers.push_back(male_speakers[0]);	// 3 more copies of target speaker index
					}
				else {// female
					message_speakers.push_back(female_speakers[0]);
					for(int i = 1; i != 4; i++)
						message_speakers.push_back(female_speakers[0]);	// 3 more copies of target speaker index
					}
				break;
				}
			default:
				Assert("invalid condition_index");
				break;
			}
		}
	
	// create the messages, target message should be first one
	for(int i = 0; i < n_speakers; i++) {
		int is = message_speakers[i]; // speaker index
//		device_out << "Create message: " << i << ' ' << message_speakers[i] << ' ' << callsign_indices[i] << ' ' 
//			<< color_indices[i] << ' ' << digit_indices[i] << endl;
        messages[i] = Message(this, stream_names[i], speakers[is].gender, speakers[is].id,
                            is, callsign_indices[i], color_indices[i], digit_indices[i],
							utterance_data[is][callsign_indices[i]][color_indices[i]][digit_indices[i]],
							loudnesses[i], // baseline loudness - -12 to + 12 for target, 0 for maskers
							// kludge for Greg's 1/2/12 Markov model
	//						loudnesses[i], .1,	// each word sampled using this mean and sd for loudness
							// loudnesses[i], speakers[is].loudness_sd,	// each word sampled using this mean and sd for loudness
	//						speakers[is].pitch_mean, speakers[is].pitch_sd, // ditto pitch
							callsigns[callsign_indices[i]], colors[color_indices[i]], digits[digit_indices[i]]);
		}
/*	{
	// kludging for Gregs 1/2/12 Markov model
	double color_swap_probs[10] = {0.15, 0.25, 0.33, 0.40, 0.48, 0.31, 0.13, 0.02, 0.005, 0.001}; // extended by guess to snr = +15
	double digit_swap_prob = .01; // collapsed over snr
	// use callsign loudness for unswapped Color and Digit loudness, swap with other message if swapping
	// initial state is that loudness is the same (or almost the same) for all words in a stream
	// so if we swap the color, we use loudness from the other stream and then swap from that change or not
	// Assert(n_speakers == 2);
	// swap target with every masker
	for(int imasker = 1; imasker < n_speakers; imasker++) {
	// flip coin to decide if swap on colors
	if(unit_uniform_random_variable() < color_swap_probs[snr_index]) {
		// swap color loudness
		double temp = messages[0].loudnesses[4];
		messages[0].loudnesses[4] = messages[imasker].loudnesses[4];
		messages[imasker].loudnesses[4] = temp;
		// now flip coin to decide whether to swap digit loudnesses
		if(unit_uniform_random_variable() > digit_swap_prob) {
			// keeping the stream swap on color for digit, so swap the digit loudnesses too.
			double temp = messages[0].loudnesses[5];
			messages[0].loudnesses[5] = messages[imasker].loudnesses[5];
			messages[imasker].loudnesses[5] = temp;
			}
		// else, leave digits in their original state
		}
	else {
		// leave color stream in its original state
		// now flip coin to decide whether to swap digit loudnesses
		if(unit_uniform_random_variable() < digit_swap_prob) {
			double temp = messages[0].loudnesses[5];
			messages[0].loudnesses[5] = messages[imasker].loudnesses[5];
			messages[imasker].loudnesses[5] = temp;
			}
		// else leave digits in original unswapped state
		}
	} // for loop end
	} // block for scoping
*/	
	
	
}

void Brungart_device::present_stimulus()
{
	create_messages();
	
	// note the time, then start the two messages
	stimulus_onset_time = get_time();
	word_counter = 0;
	state = NEXT_WORD;
	present_next_word();
}


void Brungart_device::present_next_word()
{
	// did we say the last word?
	if(word_counter == message_length_c) {
		// enable the response after a delay
		state = ENABLE_RESPONSE;
		schedule_delay_event(response_enable_delay_time_c);
		return;
		}
	Assert(word_counter < message_length_c);
		
	long duration = Message::get_duration(word_counter);

	// all durations are the same across messages

	// say the next words, target/masker speaker characteristics
	for(int i = 0; i < n_speakers; i++) {
		// have each message generate its next word
		messages[i].present_word(trial, word_counter);
		}
	// if this is the first word, supply the location corresponding to the source
	schedule_delay_event(duration + 10);  // put a bit of space after each word
	word_counter++;
}

void Brungart_device::present_response_objects()
{
	// testing - not part of the experiment
//	make_auditory_sound_stop(beep_name);
	// created at start up remains 
	//destroy_auditory_source(Symbol("Loudspeaker"));
	for(int i = 0; i < response_objects.size(); i++) {
		present_response_object(response_objects[i]);
		}
	
	// only response allows is a color-digit response, 
	// and the correct one is always the one designated as the response_target.
//	correct_response_object = response_target->name;
}

void Brungart_device::present_response_object(Response_object& response_object)
{	
	// create the object name	
	ostringstream oss;
	// trial number is final part of name, ensuring a unique name across trials
	oss << "R" << response_object.id << "_" << trial;
	response_object.name = Symbol(oss.str());
	make_visual_object_appear(response_object.name, response_object.location, GU::Size(1.0, 1.0));
	set_visual_object_property(response_object.name, Color_c, response_object.color);
	set_visual_object_property(response_object.name, Shape_c, Square_c);
	set_visual_object_property(response_object.name, Text_c, response_object.label);
}

void Brungart_device::remove_response_objects()
{
	for(int i = 0; i < response_objects.size(); ++i) {
		make_visual_object_disappear(response_objects[i].name);
		}
//	make_visual_object_disappear(None_response_object);
}

// here if a ply event is received
void Brungart_device::handle_Ply_event(const Symbol& cursor_name, const Symbol& target_name,
		GU::Point new_location, GU::Polar_vector)
{
	if(state != WAITING_FOR_RESPONSE)
		throw Device_exception(this, "Ply received while not waiting for a response");
	
	// update the cursor position
	// sanity check
	Assert(Cursor_name_c == cursor_name);
	cursor_location = new_location;
	set_visual_object_location(Cursor_name_c, cursor_location);
	current_pointed_to_object = target_name;
	if(get_trace() && Trace_out && current_pointed_to_object != "nil")
		Trace_out << processor_info() << "Ply to: " << current_pointed_to_object << endl;
	else if(get_trace() && Trace_out) // shows intermediate points
		Trace_out << processor_info() << "Ply to: " << current_pointed_to_object << endl;
}

// here if a keystroke event is received
void Brungart_device::handle_Keystroke_event(const Symbol& key_name)
{
	if(state != WAITING_FOR_RESPONSE)
		throw Device_exception(this, "Keystroke received while not waiting for a response");
	if(get_trace() && Trace_out)
		Trace_out << processor_info() << "Keystroke: " << key_name << endl;
	
	rt = get_time() - stimulus_onset_time;
		
	remove_response_objects();

	trial++;
	
	// score the response
	score_response();
	
//	output_statistics();
	if(!(trial % 100))
		device_out << processor_info() << "Trial " << trial << endl;

	if(trial >= n_trials) {
		output_statistics();
		if(setup_next_run()) {
			stop_simulation();
			return;
			}
		}
		
	state = START_TRIAL;
	schedule_delay_event(iti_c + random_int(100));
}

void Brungart_device::set_parameter(const string& proc_name, const string& param_name, const string& spec, double value)
{
	ostringstream oss;
	oss << spec << ' ' << value;
	// set the associated human parameter
	set_human_parameter(proc_name, param_name, oss.str());
}

void Brungart_device::score_response()
{

	// identify the pointed-to object
	Response_objects_t::iterator it;
	for(it = response_objects.begin(); it != response_objects.end(); ++it) {
		if(it->name == current_pointed_to_object)
			break;
		}
		
	Symbol response_color = it->color;
	Symbol response_digit = it->label;
		
	bool color_correct = (response_color == target_color);
	bool digit_correct = (response_digit == target_digit);
	
	if(color_correct && digit_correct)
		n_completely_correct++;
	else if(color_correct && !digit_correct)
		n_color_only_correct++;
	else if(!color_correct && digit_correct)
		n_digit_only_correct++;
	else if(!color_correct && !digit_correct)
		n_completely_incorrect++;

	Assert(n_completely_correct+n_color_only_correct+n_digit_only_correct+n_completely_incorrect == trial);
	
	if(!(trial % 100))
			device_out << processor_info()
			<< " Trial: " << trial << " masker speaker: " << masking_condition_labels[condition_index] << " target SNR: " << target_snrs[snr_index]
			<<" N. target: both, color, digit, neither: " 
			<< n_completely_correct << ' ' << n_color_only_correct << ' ' << n_digit_only_correct << ' ' << n_completely_incorrect 
			<< " rt: " << rt << endl;


	// find out if color is a masker color, likewise for digit
	bool masker_color = false;
	for(int i = 1; i < n_speakers; i++)
		masker_color = masker_color || (response_color == messages[i].color);
		
	Assert(!(color_correct && masker_color));
		
	bool masker_digit = false;
	for(int i = 1; i < n_speakers; i++)
		masker_digit = masker_digit || (response_digit == messages[i].digit);
	
	Assert(!(digit_correct && masker_digit));
	
	if(masker_color && masker_digit)
		n_masker_both++;
	else if(masker_color && !masker_digit)
		n_masker_color_only++;
	else if(!masker_color && masker_digit)
		n_masker_digit_only++;
	else if(!masker_color && !masker_digit)
		n_masker_neither++;	

	if(!(trial % 100))
		device_out << " masker: " << n_masker_both << ' ' << n_masker_color_only << ' ' << n_masker_digit_only << ' ' << n_masker_neither << endl;

	if(color_correct)
		n_target_color++;
	else if(masker_color)
		n_masker_color++;
	else 
		n_neither_color++;

	if(digit_correct)
		n_target_digit++;
	else if(masker_digit)
		n_masker_digit++;
	else 
		n_neither_digit++;
		
	// calculate subscripts for contingency table 0 is color/digit correct, 1 is masker, 2 is neither
	int icr = (color_correct) ? 0 : ((masker_color) ? 1 : 2);
	int idr = (digit_correct) ? 0 : ((masker_digit) ? 1 : 2);
	n_color_digit_table[icr][idr]++;
	
		
	if(!(trial % 100))
		device_out << " target-masker-neither color/digit: " 
			<< n_target_color << ' ' << n_masker_color << ' ' << n_neither_color << ' '
			<< n_target_digit << ' ' << n_masker_digit << ' ' << n_neither_digit << endl;
	
}


void Brungart_device::output_statistics()
{
	// output the results
	Assert(n_completely_correct+n_color_only_correct+n_digit_only_correct+n_completely_incorrect == n_trials);
	
	device_out 
//		<< "Trials: " << n_trials << " P(Content masked): " << content_masking_probs[condition_index] << " P(Stream masked): " << stream_masking_probs[snr_index]
//		<< "Trials: " << n_trials << " masker gender: " << masker_genders[condition_index] << " P(Stream masked): " << stream_masking_probs[snr_index]
		<< "Trials: " << n_trials << " masker speaker: " << masking_condition_labels[condition_index] << " target SNR: " << target_snrs[snr_index]
		<< "\nTarget proportions correct: both, color-only, digit-only, neither, all color, all digit:\n"
		<< double(n_completely_correct)/n_trials << ", " << double(n_color_only_correct)/n_trials   << ", " 
		<< double(n_digit_only_correct)/n_trials  << ", " 
		<< double(n_completely_incorrect)/n_trials  << ", " 
		<< double(n_completely_correct + n_color_only_correct)/n_trials  << ", " 
		<< double(n_completely_correct + n_digit_only_correct)/n_trials 
		<< endl;

	device_out << "Masker proportions: both, color-only, digit-only, neither, all color, all digit:\n"
			<< double(n_masker_both)/n_trials << ", " << double(n_masker_color_only)/n_trials   << ", " 
			<< double(n_masker_digit_only)/n_trials  << ", " 
			<< double(n_masker_neither)/n_trials  << ", " 
			<< double(n_masker_both + n_masker_color_only)/n_trials  << ", " 
			<< double(n_masker_both + n_masker_digit_only)/n_trials 
			<< endl;

	device_out << "Target/masker/neither proportions: all color, all digit:\n"
			<< double(n_target_color)/n_trials << "\t" << double(n_masker_color)/n_trials   << "\t" << double(n_neither_color)/n_trials  << "\t" 
			<< double(n_target_digit)/n_trials  << "\t" << double(n_masker_digit)/n_trials  << "\t" << double(n_neither_digit)/n_trials
			<< endl;

	device_out << "Color (rows) Digit (columns) contingency table: Target, Masker, Neither:" << endl;
	char label[3] = {'T', 'M', 'N'};
	for(int idr = 0; idr < 3; idr++)
		device_out << "\t" << label[idr] ;
	device_out << endl;
	for(int icr = 0; icr < 3; icr++) {
		device_out << label[icr] << "\t";
		for(int idr = 0; idr < 3; idr++) {
			device_out << n_color_digit_table[icr][idr] << "\t";
			}
		device_out << endl;
		}

//		output_file << n_trials << "\t" << masker_genders[condition_index] << "\t" << stream_masking_probs[snr_index] << "\t"
	output_file << n_trials << "\t" << masking_condition_labels[condition_index] << "\t" << target_snrs[snr_index] << "\t"
		<< double(n_completely_correct)/n_trials << "\t" << double(n_target_color)/n_trials << "\t" << double(n_masker_color)/n_trials   << "\t" << double(n_neither_color)/n_trials  << "\t" 
		<< double(n_target_digit)/n_trials  << "\t" << double(n_masker_digit)/n_trials  << "\t" << double(n_neither_digit)/n_trials;
	for(int icr = 0; icr < 3; icr++) {
		for(int idr = 0; idr < 3; idr++) {
			output_file  << "\t" << n_color_digit_table[icr][idr];
			}
		}
	output_file << endl;
					
}


