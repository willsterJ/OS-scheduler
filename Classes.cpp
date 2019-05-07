#include "Classes.h"

// Process------------------------------------------------------------------------
Process::Process(int inpid){
	AT = -1; TC = -1; CB = -1; IO = -1;
	pid = inpid;
	run_start_time = -1;
	cpu_time = 0;
	remaining_cpu_burst = -1;
	dynamic_PRIO = PRIO;
	CW = 0;
	IT = 0;
}


// Event-----------------------------------------------------------------
Event::Event(int time,Process *inprocess,int trans){
	process = inprocess;
	timestamp = time;
	transition = trans;
}

// DES Layer----------------------------------------------------------------
DES_Layer::DES_Layer(){
}
Event* DES_Layer::get_event(){
	Event *e = nullptr; // by default if eventqueue is empty
	if (eventQueue.empty() == false){	// if eventqueue is not empty
		e = eventQueue[0];	// extract front of queue
		eventQueue.erase(eventQueue.begin()); // delete front
	}
	return e;
}

void DES_Layer::add_event(Event *e){
	int i=0;
	while (i < eventQueue.size() && e->timestamp >= eventQueue[i]->timestamp) {
		i++;
	}
	eventQueue.insert(eventQueue.begin()+i, e);
}

int DES_Layer::get_next_time(){
	if (eventQueue.empty() == true)
		return -1;
	return eventQueue.at(0)->timestamp;
}

Event* DES_Layer::find_next_event_by_process(Process *p){
	for (int i=0; i<eventQueue.size(); i++){
		if (p == eventQueue.at(i)->process){
			return eventQueue.at(i);
		}
	}
	return nullptr;
}
void DES_Layer::remove_event(Event *e){
	for (int i=0; i<eventQueue.size(); i++){
		// if event pointers are the same
		if (eventQueue.at(i) == e){
			eventQueue.erase(eventQueue.begin() + i);
		}
	}
}

// Schedulers-----------------------------------------------------------------------------
FCFS :: FCFS(){
	scheduler_type = "FCFS";
}
void FCFS::add_process(Process *p){
	ready_queue.push_back(p);
}
Process* FCFS::get_next(){
	if (ready_queue.empty()){
		return nullptr;
	}
	Process* p = ready_queue.front();
	ready_queue.erase(ready_queue.begin());
	return p;
}

LCFS :: LCFS(){
	scheduler_type = "LCFS";
}
void LCFS :: add_process (Process *p){
	ready_queue.push_back(p);
}
Process* LCFS :: get_next(){
	if (ready_queue.empty()){
		return nullptr;
	}
	Process* p = ready_queue.back();
	ready_queue.erase(ready_queue.end() - 1);
	return p;
}

SRTF :: SRTF(){
	scheduler_type = "SRTF";
}
void SRTF :: add_process(Process *p){
	ready_queue.push_back(p);
	sort(ready_queue.begin(), ready_queue.end(), mycomparator); // sort the queue in ascending order
}
Process* SRTF::get_next(){
	if (ready_queue.empty()){
		return nullptr;
	}
	Process* p = ready_queue.front();
	ready_queue.erase(ready_queue.begin());
	return p;
}
bool SRTF :: mycomparator(Process *p1, Process *p2){  // custom comparator function used in the sort func
	int rem1 = p1->TC - p1->cpu_time;
	int rem2 = p2->TC - p2->cpu_time;
	return rem1 <= rem2;
}

RR :: RR(int quantum){
	scheduler_type = "RR " + to_string(quantum);
}
void RR :: add_process(Process *p){
	ready_queue.push_back(p);
}
Process* RR :: get_next(){
	if (ready_queue.empty()){
		return nullptr;
	}
	Process* p = ready_queue.front();
	ready_queue.erase(ready_queue.begin());
	return p;
}

PRIO :: PRIO(int quantum, int priolvl){
	scheduler_type = "PRIO " + to_string(quantum);
	this->priolvl = priolvl;

	// initialize 2D vectors
	for (int i=0; i<priolvl; i++){
		vector<Process*> activeinit;
		active_queues.push_back(activeinit);

		vector<Process*> expiredinit;
		expired_queues.push_back(expiredinit);
	}
}
void PRIO :: add_process(Process *p){
	// if process has exhausted activequeues (p's dynamic_prio = -1)
	if (p->dynamic_PRIO == -1){
		p->dynamic_PRIO = p->PRIO;
		expired_queues.at(p->PRIO).push_back(p);  // reset to original prio and push to expired queue
		return;
	}

	active_queues.at(p->dynamic_PRIO).push_back(p);
}
Process* PRIO :: get_next(){
	// check if queues are empty
	if (all_queues_empty()){
		return nullptr;
	}
	swap_if_active_empty();

	Process *p = nullptr;
	for (int i=active_queues.size()-1; i>=0; i--){
		if (!active_queues.at(i).empty()){
			p = active_queues.at(i).front();
			active_queues.at(i).erase(active_queues.at(i).begin());
			break;
		}
	}
	if (p == nullptr) printf("ALL ACTIVE QUEUES EMPTY??"); // test error check
	return p;
}
bool PRIO :: all_queues_empty(){
	for (int i=0; i<active_queues.size(); i++){
		if (!active_queues.at(i).empty()){
			return false;
		}
	}
	for (int i=0; i<expired_queues.size(); i++){
		if (!expired_queues.at(i).empty()){
			return false;
		}
	}
	return true;
}
void PRIO::swap_if_active_empty(){
	bool flag = true;
	for (int i=0; i<active_queues.size(); i++){
		if (!active_queues.at(i).empty()){
			flag = false;
		}
	}
	// if active queues are empty, swap active and expired queues
	if (flag == true){
		vector<vector<Process*>> temp;
		temp = active_queues;
		active_queues = expired_queues;
		expired_queues = temp;
	}
}

EPRIO :: EPRIO(int quantum, int priolvl, bool verbose) : PRIO(quantum, priolvl){
	scheduler_type = "PREPRIO " + to_string(quantum);
	this->verbose = verbose;
}
void EPRIO ::eprio_prempt(int current_time, Process *current, Process *running, DES_Layer *des){
	if (running != nullptr){
		if (verbose) printf("---> PRIO preemption %d by %d ?", running->pid, current->pid);

		if (current->dynamic_PRIO > running->dynamic_PRIO && find_event_at_timestamp(current_time, running, des) == nullptr){
			if (verbose) printf(" YES\n");
			// find process' next event and remove it
			Event *e = des->find_next_event_by_process(running);
			des->remove_event(e);
			// account for time difference and add back quantum time
			int time_diff = current_time - running->run_start_time;
			running->cpu_time = running->prev_cpu_time + time_diff;
			running->remaining_cpu_burst = running->prev_rem_cpu_burst - time_diff;
			// add prempt event for current timestamp
			Event *preempt_e = new Event(current_time, running, TRANS_TO_PREEMPT);
			preempt_e->created_time = e->created_time;
			preempt_e->prev_state = STATE_RUNNING;
			des->eventQueue.insert(des->eventQueue.begin(), preempt_e);
		}
		else{
			if (verbose) printf(" NO\n");
		}
	}
}
// find running 
Event* EPRIO::find_event_at_timestamp(int current_time, Process *p, DES_Layer *des){
	Event *e = nullptr;
	for (int i=0; i<des->eventQueue.size(); i++){
		Event *curr = des->eventQueue.at(i);
		if (curr->timestamp == current_time && p == curr->process){
			e = curr;
			break;
		}
	}
	return e;
}



// IO_Box-------------------------------------------------------------------------------------
IO_Box::IO_Box(){
	isActive = false;
	totalTime = 0;
	endTime = 0;
}
void IO_Box::checkIO(int current_time, int burst){
	if (isActive == false){
		isActive = true;
		totalTime += burst;
		endTime = current_time + burst;
	}
	else{
		int io_time = current_time + burst;
		if (io_time >= endTime){
			totalTime += io_time - endTime;
			endTime = io_time;
		}
	}
}
void IO_Box::refresh(int current_time){
	if (current_time >= endTime){
		isActive = false;
	}
}