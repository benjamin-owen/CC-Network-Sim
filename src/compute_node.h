#ifndef __COMPUTE_NODE_H__
#define __COMPUTE_NODE_H__

#include "stdint.h"
#include "port.h"
#include "math.h"
#include "packet.h"
#include <stdio.h>

// definitions for memory node
#define CMP_NUM_BOT_PORTS	1	// should not ever be changed
#define CMP_QUEUE_SIZE		256	// in packets
#define BLOCK_SIZE			1
#define WORD_SIZE			32
#define CACHE_LINES			4

typedef enum {
	INVALID,
	SHARED,
	MODIFIED,
} state_t;

// struct holding data inside a ComputeNode
typedef struct ComputeNodeMemoryLine
{
	uint32_t address;
	uint32_t value;
	// status of all compute nodes
	uint8_t valid;
	uint8_t dirty;
	state_t state;
}
ComputeNodeMemoryLine;

// top-level struct for a memory node
typedef struct ComputeNode
{
	uint8_t id;
	uint32_t time;
	// array of ports (queues) -> needs to contain a Packet struct type
	// array of MemoryLines
	Port bot_ports[CMP_NUM_BOT_PORTS];
	ComputeNodeMemoryLine cache[CACHE_LINES];
	// last used cache line (for LRU policy)
	int last_used;
}
ComputeNode;

void updateNodeState(ComputeNode* node);
int read_action(ComputeNode node, uint32_t address);
void write_action(ComputeNode* node, uint32_t address, uint32_t wdata);
void cnode_process_packet(ComputeNode* node, Packet pkt, int* stall);
void print_cache(ComputeNode* node);
void get_compute_control_count();


#endif