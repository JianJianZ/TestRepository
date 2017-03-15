// External definitions for single server queue system. 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <deque>
using namespace std;

#define BUSY 1// Mnemonics for server's being busy
#define IDLE 0// and idle. 

enum event_type { ArrivalEvent = 1, DepartureEvent, EndEvent };

struct eRecord    //record for future event
{
	float eTime;//event time
	int eType;//event type
};

int next_event_type, num_custs_delayed, num_in_q, MQ, server_status;
float area_num_in_q, area_server_status;
float sim_time, time_last_event, total_of_delays;
FILE *InterArrivalFile, *ServiceTimeFile, *outfile;

deque<float> qInterArrivalTime; //queue for interarrival time
deque<float> qArrivalTime;//queue for Arrival time
deque<float> qServiceTime;//queue for service time
deque<eRecord*> FElist;//future event list

void ReadDataFromFiles();
void closeFiles();
void initialize(void);
void timing(void);
void arrive(void);
void depart(void);
void update_time_avg_stats(void);
void report(void);
void printTable();
void insertFElist(eRecord *e);

void ReadDataFromFiles()
{
	// Open input and output files. 
	fopen_s(&InterArrivalFile, "arrival.txt", "r");
	if (InterArrivalFile == NULL) exit(-1);
	fopen_s(&ServiceTimeFile, "depart.txt", "r");
	if (ServiceTimeFile == NULL) exit(-1);
	fopen_s(&outfile, "mm1.out", "w");
	if (outfile == NULL) exit(-1);

	// Read arrival times to  qInterArrivalTime
	while (!feof(InterArrivalFile)) {
		float x;
		fscanf_s(InterArrivalFile, "%f", &x);
		qInterArrivalTime.push_back(x);
	}

	// Read departure times to  qServiceTime. 
	while (!feof(ServiceTimeFile)) {
		float x;
		fscanf_s(ServiceTimeFile, "%f", &x);
		qServiceTime.push_back(x);
	}
}

void closeFiles()
{
	fclose(ServiceTimeFile);
	fclose(InterArrivalFile);
	fclose(outfile);
}

int main() {
	ReadDataFromFiles();
	fprintf(outfile, "HW5: Single-server queueing system simulation\n\n");
	initialize();
	fprintf(outfile, "\nClock \tLQ \tLS \tB \t\tMQ \tFE List\n");
	do {
		printTable();
		timing();
		update_time_avg_stats();
		switch (next_event_type) {
		case ArrivalEvent:
			arrive();
			break;
		case DepartureEvent:
			depart();
			break;
		case EndEvent:
			break;
		}
	} while (next_event_type != EndEvent);
	report();
	closeFiles();
	return 0;
}

void initialize(void) {
	sim_time = 0.0;
	server_status = IDLE;
	num_in_q = 0;
	MQ = 0;
	time_last_event = 0.0;
	// Initialize the statistical counters.
	num_custs_delayed = 0;
	total_of_delays = 0.0;
	area_num_in_q = 0.0;
	area_server_status = 0.0;

	// Add first arrival event
	eRecord *e = new eRecord();
	e->eTime = sim_time + qInterArrivalTime.front();
	qInterArrivalTime.pop_front();
	e->eType = ArrivalEvent;
	FElist.push_back(e);
	//add end simulation event
	eRecord *e2 = new eRecord();
	e2->eTime = 20;
	e2->eType = EndEvent;
	FElist.push_back(e2);
}

void timing(void) {
	next_event_type = 0;

	// Determine the event type of the next event to occur. 
	deque<eRecord*>::const_iterator iter;
	iter = FElist.begin();
	if (iter != FElist.end()) {
		eRecord *e = FElist.front();
		sim_time = e->eTime;
		next_event_type = e->eType;
		FElist.erase(iter);
	}

	// Check to see whether the event list is empty.
	if (next_event_type == 0) {
		// The event list is empty, so stop the simulation. 
		fprintf(outfile, "\nEvent list empty at time %f", sim_time);
		exit(1);
	}
}

void arrive(void) {
	float delay;
	// Schedule next arrival event
	if (qInterArrivalTime.size() > 0) {
		//insert into Future Event list
		eRecord *e = new eRecord();
		e->eTime = sim_time + qInterArrivalTime.front();
		qInterArrivalTime.pop_front();
		e->eType = ArrivalEvent;
		insertFElist(e);
	}

	// Check to see whether server is busy. 
	if (server_status == BUSY) {
		// Server is busy, so increment number of customers in queue. 
		++num_in_q;
		if (MQ < num_in_q)
			MQ = num_in_q;
		// store the time of arrival of the arriving customer at the (new) end.
		qArrivalTime.push_back(sim_time);
	}
	else {
		// Server is idle, so arriving customer has a delay of zero. 
		delay = 0.0;
		total_of_delays += delay;

		// Increment the number of customers delayed, and make server busy. 
		++num_custs_delayed;
		server_status = BUSY;

		// Schedule a departure (service completion). 
		if (qServiceTime.size() > 0) {
			//insert departure event into Future Event list
			eRecord *e = new eRecord();
			e->eTime = sim_time + qServiceTime.front();
			qServiceTime.pop_front();

			e->eType = DepartureEvent;
			insertFElist(e);
		}
	}
}

void printTable() {
	string format = "%.0f \t\t%d \t%d \t%.01f \t\t%d ";
	fprintf(outfile, format.c_str(),
		sim_time,
		num_in_q,
		server_status,
		area_server_status,
		MQ);

	deque<eRecord*>::const_iterator iter;
	for (iter = FElist.begin(); iter != FElist.end(); iter++) {
		eRecord *p = iter.operator*();
		fprintf(outfile, " (%d,%.0f)",
			p->eType,
			p->eTime);
	}
	fprintf(outfile, "\n");
}

void depart(void)
{
	float delay;
	// Check to see whether the queue is empty.
	if (num_in_q == 0) {
		// The queue is empty so make the server idle
		server_status = IDLE;
	}
	else {
		--num_in_q;
		delay = sim_time - qArrivalTime.front();
		if (delay < 0)  delay = 0;
		//update the total delay accumulator.
		total_of_delays += delay;
		// Increment the number of customers delayed
		++num_custs_delayed;
		// Move each customer in queue (if any) up one place. 
		qArrivalTime.pop_front();
		//schedule departure. 
		if (qServiceTime.size() > 0) {
			//insert departure event into Future Event list
			eRecord *e = new eRecord();
			e->eTime = sim_time + qServiceTime.front();
			qServiceTime.pop_front();
			e->eType = DepartureEvent;
			insertFElist(e);
		}
	}
}

void update_time_avg_stats(void) {
	// Compute time since last event, and update last-event-time marker. 
	float time_since_last_event = sim_time - time_last_event;

	if (time_since_last_event < 0)
		time_since_last_event = 0;

	time_last_event = sim_time;

	// Update area under number-in-queue function.
	area_num_in_q += num_in_q * time_since_last_event;

	// Update area under server-busy indicator function.
	area_server_status += server_status * time_since_last_event;
}

void report(void) {
	fprintf(outfile, "\nAverage delay in queue%11.2f minutes\n",
		total_of_delays / num_custs_delayed);
	fprintf(outfile, "Average number in queue%10.2f\n",
		area_num_in_q / sim_time);
	fprintf(outfile, "Server utilization%15.2f\n",
		area_server_status / sim_time);
	fprintf(outfile, "Number of delays completed%7d",
		num_custs_delayed);
}

void insertFElist(eRecord *e) {
	deque<eRecord*>::const_iterator iter;
	for (iter = FElist.begin(); iter != FElist.end(); iter++) {
		eRecord *p = iter.operator*();
		if (p->eTime >= e->eTime) {
			FElist.insert(iter, e);
			break;
		}
	}
}

