#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "Classes.h"
using namespace std;

// global variables -----------------------------------------------------------

vector<int> random_vect;	// to store vector of all randum nums in file
int random_offset;	// offset for randoms
vector<Process> processList;	// list of processes 


// functions ------------------------------------------------------------------
void read_rand(string name){
	ifstream file;
	string line;
	file.open(name);
	random_offset = 0;
	if (file.is_open()){
		while (getline(file, line)){
			random_vect.push_back(stoi(line));	// store random numbers in vector
		}
	}
	else cout << "Unable to open randfile" << endl;
	file.close();
}

int myrandom(int burst) {
	if (random_offset == random_vect.size())	// if random list is exhausted, start at beginning
		random_offset = 0;
	return 1 + (random_vect[++random_offset] % burst); 
}

void read_file(string filename, int maxprio){
	ifstream file;
	file.open(filename);
	string token;
	int process_id = 0;

	if (file.is_open()){
		while(file >> token){
			Process *p = new Process(process_id++);

			p->PRIO = myrandom(maxprio) - 1;

			p->AT = stoi(token);
			file >> token;
			p->TC = stoi(token);
			file >> token;
			p->CB = stoi(token);
			file >> token;
			p->IO = stoi(token);

			processList.push_back(*p);
		}
	} else cout << "Unable to open input file" << endl;
	file.close();
}

string State_enum_to_string (int state){
	string out;
	if (state == STATE_CREATED) out = "CREATED";
	else if (state == STATE_READY) out = "READY";
	else if (state == STATE_RUNNING) out = "RUNNG";
	else if (state == STATE_BLOCKED) out = "BLOCK";
	else if (state == STATE_PREEMPTED) out = "PREMPT";

	return out;
}

// main-------------------------------------------------------------------------------

int main(int argc, char* argv[]){

	bool verbose = false;
	string filename;
	string randfilename;
	int quantum;
	Scheduler *scheduler = nullptr;
	bool E_scheduler = false;
	int maxprio = 4;


	// go through the commands first
	int i;
	for (i=1; i<sizeof(argv)+1; i++){	// i=0 is the .exe file, so skip
		string command = argv[i];
		if (command.compare("-v") == 0){
			verbose = true;
			continue;
		}
		else if (command.compare("-t") == 0) continue;
		else if (command.compare("-e") == 0) continue;
		
		else if (argv[i][1] == 's' && strlen(argv[i]) < 10){
			char spec = argv[i][2];
			if (spec == 'F'){
				scheduler = new FCFS();
				quantum = 10000;
			}
			else if (spec == 'L'){
				scheduler = new LCFS();
				quantum = 10000;
			}
			else if (spec == 'S'){
				scheduler = new SRTF();
				quantum = 10000;
			}
			else if (spec == 'R'){
				string strnum;
				for (int j=3; j<strlen(argv[i]); j++){
					strnum += argv[i][j];
				}
				quantum = atoi(strnum.c_str());
				scheduler = new RR(quantum);
			}
			else if (spec == 'P'){
				string str;
				for (int j=3; j<strlen(argv[i]); j++){
					str += argv[i][j];
				}
				sscanf_s(str.c_str(),"%d_%d",&quantum,&maxprio);
				scheduler = new PRIO(quantum, maxprio);
			}
			else if (spec == 'E'){
				string str;
				for (int j=3; j<strlen(argv[i]); j++){
					str += argv[i][j];
				}
				sscanf_s(str.c_str(),"%d_%d",&quantum,&maxprio);
				scheduler = new EPRIO(quantum, maxprio,verbose);
				E_scheduler = true;
			}
		}
		else{
			std::string name(argv[i]);
			filename = name;
			i++;
			std::string name2(argv[i]);
			randfilename = name2;

			continue;
		}
	}
	
	read_rand(randfilename);	// read rand file
	read_file(filename, maxprio);	// read input file

	// initialize DES layering
	DES_Layer *des = new DES_Layer();
	for (i=0; i<processList.size(); i++){
		Event *e = new Event(processList[i].AT, &processList[i], TRANS_TO_READY);
		processList[i].state = STATE_CREATED;
		e->process = &processList[i];
		e->process->pid = i;
		e->process->dynamic_PRIO = e->process->PRIO;
		e->created_time = processList[i].AT;
		e->prev_state = STATE_CREATED;
		des->add_event(e);
	}
	
	// Simulation initial vars
	int current_time=0;
	bool CALL_SCHEDULER = false;
	Process* running_process = nullptr;
	int cpu_burst = NULL;
	int io_burst = NULL;
	int last_event_time;
	IO_Box *io_box = new IO_Box();


	// begin Simulation
	Event *e = nullptr;
	for (e = des->get_event(); e != nullptr; e = des->get_event()){

		if (e->timestamp == 268) // for debug
			printf("");

		Process* current_process = e->process;
		current_time = e->timestamp;

		if (verbose) printf("%d %d", current_time, e->process->pid);

		switch(e->transition){
		case TRANS_TO_READY:
		{
			if (verbose) printf(" %d: %s -> READY\n", current_time - e->created_time, State_enum_to_string(e->prev_state).c_str());

			if (E_scheduler == true && (current_process->state == STATE_BLOCKED || current_process->state == STATE_CREATED)){
				dynamic_cast<EPRIO*>(scheduler)->eprio_prempt(current_time, current_process, running_process, des);
			}

			current_process->ready_start_time = current_time;
			current_process->state = STATE_READY;
			scheduler->add_process(current_process);
			CALL_SCHEDULER = true;

		}break;
		case TRANS_TO_RUN:
		{
			if (verbose) printf(" %d: %s -> RUNNG", current_time - current_process->ready_start_time, State_enum_to_string(e->prev_state).c_str());
			running_process = current_process;
			current_process->state = STATE_RUNNING;
			current_process->CW += current_time - current_process->ready_start_time;

			// begin run start time
			if (current_process->run_start_time == -1)
				current_process->run_start_time = current_time;

			// check if process still has cpu burst left (after being preempted)
			if (current_process->remaining_cpu_burst > 0){
				cpu_burst = current_process->remaining_cpu_burst;
			}
			else{
				cpu_burst = myrandom(running_process->CB);
			}
			// check for near cpu completion
			if (cpu_burst >= current_process->TC - current_process->cpu_time)
				cpu_burst = current_process->TC - current_process->cpu_time;

			current_process->remaining_cpu_burst = cpu_burst;	// update

 			if (verbose) printf(" cb=%d rem=%d prio=%d\n", cpu_burst, current_process->TC - current_process->cpu_time, current_process->dynamic_PRIO);

			// save previous cpu time and remaining cpu burst. To be used in EPRIO
			current_process->prev_cpu_time = current_process->cpu_time;
			current_process->prev_rem_cpu_burst = current_process->remaining_cpu_burst;

			Event *e = nullptr;
			// check for preempt
			if (cpu_burst > quantum){
				current_process->cpu_time += quantum;
				current_process->remaining_cpu_burst = cpu_burst - quantum;
				e = new Event(current_time + quantum, current_process, TRANS_TO_PREEMPT);
			}
			// check for block
			else if (cpu_burst + current_process->cpu_time < current_process->TC){
				//current_process->remaining_cpu_burst = -1;
				current_process->cpu_time += cpu_burst;
				e = new Event(current_time + cpu_burst, current_process, TRANS_TO_BLOCK);
			}

			// check for done
			else if (cpu_burst + current_process->cpu_time >= current_process->TC){
				current_process->cpu_time += cpu_burst;
				e = new Event(current_time + cpu_burst, current_process, TRANS_TO_DONE);
			}

			e->created_time = current_time;
			e->prev_state = STATE_RUNNING;
			des->add_event(e);

		}break;
		case TRANS_TO_BLOCK:
		{
			// reset burst and time
			current_process->remaining_cpu_burst = -1;
			current_process->run_start_time = -1;
			current_process->dynamic_PRIO = current_process->PRIO;

			if (verbose) printf(" %d: %s -> BLOCK", current_time - e->created_time, State_enum_to_string(e->prev_state).c_str());

			if (running_process == nullptr) cout << "PROBLEM: TRANS_TO_BLOCK running process is null" << endl;
			running_process = nullptr;
			current_process->state=STATE_BLOCKED;

			io_burst = myrandom(current_process->IO);
			current_process->IT += io_burst;
			
			if (verbose) printf("  ib=%d rem=%d\n", io_burst, current_process->TC - current_process->cpu_time);

			Event *e = new Event(current_time + io_burst, current_process, TRANS_TO_READY);
			e->created_time = current_time;
			e->prev_state = STATE_BLOCKED;
			des->add_event(e);


			// check if IO is available
			io_box->refresh(current_time);
			io_box->checkIO(current_time, io_burst);  // if io resource is in use, allocate what remains

			CALL_SCHEDULER = true;
			
		}break;
			
		case TRANS_TO_PREEMPT:
		{
			if (verbose) printf(" %d: %s -> READY  cb=%d rem=%d prio=%d\n", (current_time - e->created_time), State_enum_to_string(e->prev_state).c_str(), current_process->remaining_cpu_burst, current_process->TC - current_process->cpu_time, current_process->dynamic_PRIO);

			current_process->state = STATE_PREEMPTED;
			current_process->dynamic_PRIO = current_process->dynamic_PRIO - 1; // drop process' prio
			current_process->run_start_time = -1;	// reset run start time

			running_process = nullptr;

			current_process->ready_start_time = current_time;
			scheduler->add_process(current_process);

			CALL_SCHEDULER = true;
			
		}break;

		case TRANS_TO_DONE:
		{
			if (verbose) printf(" %d: Done\n", current_time-e->created_time);
			
			running_process = nullptr;

			current_process->FT = current_time;
			last_event_time = current_time;
			CALL_SCHEDULER = true;

		}break;
			
		}
		
		
		if (CALL_SCHEDULER){
			if (des->get_next_time() == current_time) continue;
			CALL_SCHEDULER = false;
			if (running_process == nullptr){
				running_process = scheduler->get_next();
				if (running_process == nullptr) continue;
				Event *e = new Event(current_time, running_process, TRANS_TO_RUN);
				e->created_time = current_time;
				e->prev_state = STATE_READY;
				des->add_event(e);

				running_process = nullptr;
			}
		}
	}

	// display info
	cout << scheduler->scheduler_type << endl;

	int total_cpu = 0;
	int total_turnaround = 0;
	int total_cpu_wait_time = 0;

	for (int i=0; i < processList.size(); i++){
		printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",
		    i,
		    processList[i].AT, //
		    processList[i].TC,
			processList[i].CB,
			processList[i].IO,
			processList[i].PRIO+1,
			processList[i].FT,
			processList[i].FT - processList[i].AT,
			processList[i].IT,
			processList[i].CW);

		total_cpu += processList[i].TC;
		total_turnaround += (processList[i].FT - processList[i].AT);
		total_cpu_wait_time += processList[i].CW;
	}

	// calculate summary
	double cpu_util = 100 * (total_cpu/(double)last_event_time);
	double io_util = 100 * io_box->totalTime/((double)last_event_time);
	double average_turnaround = total_turnaround/(double)processList.size();
	double average_wait_time = total_cpu_wait_time/(double)processList.size();
	double throughput = 100 * processList.size()/(double)last_event_time;

	printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n",
		last_event_time,
		cpu_util,
		io_util,
		average_turnaround,
		average_wait_time,
		throughput);
	
	
	system("pause");

}