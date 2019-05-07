#include <string>
#include <vector>
#include <algorithm>

using namespace std;

enum transition_state{TRANS_TO_READY, TRANS_TO_RUN, TRANS_TO_BLOCK,TRANS_TO_PREEMPT, TRANS_TO_DONE};
enum process_state{STATE_CREATED, STATE_READY, STATE_RUNNING, STATE_BLOCKED, STATE_PREEMPTED};

class Process{
public:
	int AT;
	int TC;
	int CB;
	int IO;
	int PRIO;	// priority assigned to the process
	int FT;		// Finishing Time
	int TT;		// Turnaround Time
	int IT;		// IO Time (blocked)
	int CW;		// CPU Waiting Time
	int pid;	// process id
	int state;
	int run_start_time; // run start time
	int ready_start_time;
	int cpu_time;
	int dynamic_PRIO;
	int remaining_cpu_burst;

	int prev_cpu_time;	// keeps track of time before next event. To be used in EPRIO
	int prev_rem_cpu_burst;

	Process(int pid);
};

class Event{
public:
	int timestamp;
	Process* process;
	int transition;
	int prev_state;
	int created_time;

	Event(int time, Process *inprocess, int trans);

};

class DES_Layer{
public:
	vector<Event*> eventQueue;

	DES_Layer();
	Event* get_event();
	void add_event(Event *e);
	int get_next_time();
	Event* find_next_event_by_process(Process *p);
	void remove_event(Event *e);
};

class Scheduler{
public:
	string scheduler_type;
	vector<Process*> ready_queue;
	virtual void add_process(Process *p) = 0;
	virtual Process* get_next() = 0;
};
class FCFS : public Scheduler{
public:
	FCFS();
	void add_process(Process *p);
	Process* get_next();
};
class LCFS : public Scheduler{
public:
	LCFS();
	void add_process(Process *p);
	Process* get_next();
};
class SRTF : public Scheduler{
private:
	static bool mycomparator(Process *p1, Process *p2);
public:
	SRTF();
	void add_process(Process *p);
	Process* get_next();
};
class RR : public Scheduler{
public:
	RR(int quantum);
	void add_process(Process *p);
	Process* get_next();
};
class PRIO : public Scheduler{
public:
	int priolvl;
	vector<vector<Process*>> active_queues;
	vector<vector<Process*>> expired_queues;
	PRIO(int quantum, int priolvl);
	void add_process(Process *p);
	Process* get_next();
protected:
	bool all_queues_empty();
	void swap_if_active_empty();
};
class EPRIO : public PRIO{
public:
	EPRIO(int quantum, int priolvl, bool verbose);
	void eprio_prempt(int current_time, Process *current, Process *running, DES_Layer *des);
private:
	bool verbose;
	Event* find_event_at_timestamp(int current_time, Process *p, DES_Layer *des);
};


class IO_Box{
public:
	bool isActive;
	int totalTime;
	int endTime;

	IO_Box();
	void checkIO(int current_time, int burst);
	void refresh(int current_time);
};