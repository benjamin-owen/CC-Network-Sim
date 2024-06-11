#ifndef __MEMORY_NODE_H__
#define __MEMORY_NODE_H__

#include "stdint.h"
#include "port.h"
#include "packet.h"
#include "stdlib.h"

// definitions for memory node
#define MEM_NUM_TOP_PORTS	1	// should not ever be changed
#define MEM_QUEUE_SIZE		256	// in packets
#define MEM_NUM_LINES		64
#define MEM_LINE_SIZE		32	// in bits

// enum defining various cache states for MSI protocol
typedef enum
{
	I, S, M
}
StatesMSI_t;

// struct holding data inside a MemoryNode
typedef struct MemoryLine
{
	uint32_t address;
	uint32_t value;
	// status of all compute nodes
	// StatesMSI_t* nodeState;
	StatesMSI_t nodeState[128];
}
MemoryLine;

// top-level struct for a memory node
typedef struct MemoryNode
{
	uint8_t id;
	uint32_t time;

	Port top_ports[MEM_NUM_TOP_PORTS];

	MemoryLine memory[MEM_NUM_LINES];
}
MemoryNode;

void init_memnodes(MemoryNode* node, int node_cnt);
Packet process_packet(MemoryNode* node, Packet pkt, uint32_t global_id, uint32_t global_time, uint32_t memory_node_min_id, Port* p);
void generate_invalidations(MemoryNode* node, Packet pkt, Port* p, uint32_t global_id, uint32_t global_time, uint32_t memory_node_min_id);
int get_memory_control_count();
int get_memory_to_compute_requests();

#endif