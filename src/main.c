#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"

#include "main.h"
#include "compute_node.h"
#include "switch_node.h"
#include "memory_node.h"
#include "packet.h"
#include "port.h"

int main(int argc, char ** argv)
{
	if (argc < 3)
	{
		printf(ANSI_COLOR_RED "Usage: ./sim [input file] [logfile]" ANSI_COLOR_RESET "\n");
		return EXIT_FAILURE;
	}

	// universal variables
	uint32_t global_time = 0;
	uint32_t global_id = 0;

	// debug messages
#ifdef DEBUG
	printf(ANSI_COLOR_RED     "RED"     ANSI_COLOR_RESET " ");
	printf(ANSI_COLOR_GREEN   "GREEN"   ANSI_COLOR_RESET " ");
	printf(ANSI_COLOR_YELLOW  "YELLOW"  ANSI_COLOR_RESET " ");
	printf(ANSI_COLOR_BLUE    "BLUE"    ANSI_COLOR_RESET " ");
	printf(ANSI_COLOR_MAGENTA "MAGENTA" ANSI_COLOR_RESET " ");
	printf(ANSI_COLOR_CYAN    "CYAN"    ANSI_COLOR_RESET "\n\n");
#endif

	printf("Input file: %s\n", argv[1]);
	printf("Output file: %s\n\n", argv[2]);

	// compute nodes
	ComputeNode compute_nodes[NUM_COMPUTE_NODES];
	uint32_t compute_node_min_id = global_id;
	for (int i = 0; i < NUM_COMPUTE_NODES; i++)
	{
		ComputeNode node;
		node.id = global_id;
		global_id++;
		node.time = global_time;
		for (int j = 0; j < CMP_NUM_BOT_PORTS; j++)
		{
			node.bot_ports[j].tail_tx = 0;
			node.bot_ports[j].tail_rx = 0;
		}
		compute_nodes[i] = node;
	}
	uint32_t compute_node_max_id = global_id - 1;

	// switch nodes
	SwitchNode switch_nodes[NUM_SWITCH_NODES];
	uint32_t switch_mode_min_id = global_id;
	for (int i = 0; i < NUM_SWITCH_NODES; i++)
	{
		SwitchNode node;
		node.id = global_id;
		global_id++;
		node.time = global_time;
		for (int j = 0; j < SW_NUM_TOP_PORTS; j++)
		{
			node.top_ports[j].tail_tx = 0;
			node.top_ports[j].tail_rx = 0;
		}
		for (int j = 0; j < SW_NUM_BOT_PORTS; j++)
		{
			node.bot_ports[j].tail_tx = 0;
			node.bot_ports[j].tail_rx = 0;
		}
		switch_nodes[i] = node;
	}
	uint32_t switch_node_max_id = global_id - 1;

	// memory nodes
	MemoryNode memory_nodes[NUM_MEMORY_NODES];
	uint32_t memory_node_min_id = global_id;
	for (int i = 0; i < NUM_MEMORY_NODES; i++)
	{
		MemoryNode node;
		node.id = global_id;
		global_id++;
		node.time = global_time;
		for (int j = 0; j < MEM_NUM_TOP_PORTS; j++)
		{
			node.top_ports[j].tail_tx = 0;
			node.top_ports[j].tail_rx = 0;
		}
		memory_nodes[i] = node;
	}
	uint32_t memory_node_max_id = global_id - 1;

	// parse input file
	FILE * input_file;
	input_file = fopen(argv[1], "r");
	if (input_file == NULL)
	{
		printf(ANSI_COLOR_RED "Input file does not exist!" ANSI_COLOR_RESET "\n");
		return EXIT_FAILURE;
	}
	char line[255]; // maximum line length 255 (better not be longer)
	char * token;
	while (fgets(line, sizeof(line), input_file) != NULL)
	{
		printf("Current line: %s", line);
		// create packet to be generated from this line
		DataNode data_node;
		Packet packet;
		// loop through each line in file
		int curr_field = 0;
		token = strtok(line, ",");
		while (token != NULL)
		{
		#ifdef DEBUG
			printf("Current token: %s\n", token);
		#endif
			switch (curr_field)
			{
				case 0:
				{
					// time
					packet.time = atoi(token);
					break;
				}

				case 1:
				{
					// packet ID
					packet.id = global_id++;
					break;
				}

				case 2:
				{
					// flag
					packet.flag = atoi(token);
					break;
				}

				case 3:
				{
					// src
					packet.src = atoi(token);
					break;
				}

				case 4:
				{
					// dst
					packet.dst = atoi(token);
					break;
				}

				case 5:
				{
					// address
					data_node.addr = (uint32_t) strtol(token, NULL, 16);
					break;
				}

				case 6:
				{
					// data
					data_node.data = (uint32_t) strtol(token, NULL, 16);
					break;
				}

				default:
				{
					// error
					printf(ANSI_COLOR_RED "Invalid input file format!" ANSI_COLOR_RESET "\n");
					fclose(input_file);
					return EXIT_FAILURE;
				}
			}
			token = strtok(NULL, ",");
			curr_field++;
		}
		// place packet in correct buffer
		packet.data = data_node;
		if (packet.src <= compute_node_max_id)
		{
			if (push_packet((&compute_nodes[packet.src - compute_node_min_id].bot_ports[0]), TX, packet) != 0)
			{
				printf(ANSI_COLOR_RED "Invalid input file format!" ANSI_COLOR_RESET "\n");
				fclose(input_file);
				return EXIT_FAILURE;
			}
		}
		else if (packet.src <= switch_node_max_id)
		{
			printf(ANSI_COLOR_RED "Invalid packet SRC address (switch)!" ANSI_COLOR_RESET "\n");
			fclose(input_file);
			return EXIT_FAILURE;
		}
		else if (packet.src <= memory_node_max_id)
		{
			if (push_packet((&memory_nodes[packet.src - memory_node_min_id].top_ports[0]), TX, packet) != 0)
			{
				printf(ANSI_COLOR_RED "Invalid input file format!" ANSI_COLOR_RESET "\n");
				fclose(input_file);
				return EXIT_FAILURE;
			}
		}
		else
		{
			printf(ANSI_COLOR_RED "Invalid packet SRC address!" ANSI_COLOR_RESET "\n");
			fclose(input_file);
			return EXIT_FAILURE;
		}
    }
	fclose(input_file);

	// create output file
	FILE * output_file;
	output_file = fopen(argv[2], "w");

	// TODO TESTING REMOVE
	// create test data for packet
	// DataNode data_node = { 0x0, 0x00000000 };
	// DataNode data_node_2 = { 0x200, 0x00000000 };
	// DataNode data_node_3 = { 0x100, 0x00000000 };
	// // create packet with data
	// // uint8_t dest = ((data_node.addr >> 3) / MEM_NUM_LINES) + memory_node_min_id; // TODO SAVE THIS
	// // place a packet in compute node destined to memory node
	// for (int i = 0; i < 1; i++)
	// {
	// 	Packet test_packet = { global_id++, global_time, READ, compute_nodes[0].id, 0, data_node };
	// 	if (push_packet((&(compute_nodes[0].bot_ports[0])), TX, test_packet) != 0)
	// 	{
	// 		fclose(output_file);
	// 		return EXIT_FAILURE;
	// 	}

	// 	Packet test_packet_3 = { global_id++, global_time, READ, compute_nodes[0].id, 0, data_node_3 };
	// 	if (push_packet((&(compute_nodes[1].bot_ports[0])), TX, test_packet_3) != 0)
	// 	{
	// 		fclose(output_file);
	// 		return EXIT_FAILURE;
	// 	}

	// 	Packet test_packet_2 = { global_id++, global_time, READ, compute_nodes[1].id, 0, data_node_2 };
	// 	if (push_packet((&(compute_nodes[1].bot_ports[0])), TX, test_packet_2) != 0)
	// 	{
	// 		fclose(output_file);
	// 		return EXIT_FAILURE;
	// 	}
	// }
	print_port(compute_nodes[0].bot_ports[0], TX);
	print_port(compute_nodes[1].bot_ports[0], TX);
	// initialize memory location
	MemoryLine line_1 = { 0x0, 0xDEADBEEF, SHARED };
	memory_nodes[0].memory[0] = line_1;
	MemoryLine line_2 = { 0x200, 0xABBAABBA, SHARED };
	memory_nodes[1].memory[0] = line_2;
	MemoryLine line_3 = { 0x100, 0xCDCDCDCD, SHARED };
	memory_nodes[0].memory[32] = line_3;

	printf("\nCOMPUTE NODES ID: [%d-%d], SWITCH NODES ID: [%d-%d], MEMORY NODES ID: [%d-%d]\n", compute_node_min_id, compute_node_max_id, switch_mode_min_id, switch_node_max_id, memory_node_min_id, memory_node_max_id);

	// main loop
	char finished = 1;
	do // TODO loop through packets to send
	{
		printf("\n--- GLOBAL TIME = %d\n", global_time);
		// outgoing from compute nodes
		for (int i = 0; i < NUM_COMPUTE_NODES; i++)
		{
			printf("* Compute node with ID %d at time %d\n", compute_nodes[i].id, global_time);
			// loop through outgoing ports to switch nodes
			for (int j = 0; j < CMP_NUM_BOT_PORTS; j++)
			{
				Packet curr_packet_tx = pop_packet((&(compute_nodes[i].bot_ports[j])), TX, 0);
				Packet curr_packet_rx = pop_packet((&(compute_nodes[i].bot_ports[j])), RX, 0);
				if ((curr_packet_tx.flag != ERROR) && (curr_packet_tx.time <= global_time))
				{
					// need to act on this packet
					// if ((curr_packet_tx.dst >= memory_node_min_id) && (curr_packet_tx.dst <= memory_node_max_id))
					// {
						printf(ANSI_COLOR_BLUE "Moving packet with ID %d to switch with ID %d" ANSI_COLOR_RESET "\n", curr_packet_tx.id, switch_nodes[0].id);
						curr_packet_tx.time = global_time + GLOBAL_TIME_INCR; // TODO custom value
						// place packet into switch port corresponding to compute node ID
						push_packet((&(switch_nodes[0].top_ports[i])), RX, curr_packet_tx);
					// }
					// else
					// {
					// 	printf("Packet with ID %d has invalid destination\n", curr_packet_tx.id);
					// }
					// remove packet from compute node
					pop_packet((&(compute_nodes[i].bot_ports[j])), TX, 1);
				}

				if ((curr_packet_rx.flag != ERROR) && (curr_packet_rx.time <= global_time))
				{
					// need to act on this packet
					if (curr_packet_rx.dst == compute_nodes[i].id)
					{
						printf(ANSI_COLOR_GREEN "Packet with ID %d has arrived at compute node with ID %d [0x%08x, 0x%08x]" ANSI_COLOR_RESET "\n", curr_packet_rx.id, compute_nodes[i].id, curr_packet_rx.data.addr, curr_packet_rx.data.data);
						// TODO act on this
					}
					else
					{
						printf(ANSI_COLOR_RED "Packet with ID %d has invalid destination" ANSI_COLOR_RESET "\n", curr_packet_rx.id);
					}
					// remove packet from compute node
					pop_packet((&(compute_nodes[i].bot_ports[j])), RX, 1);
				}
			}
		}

		// outgoing from switch nodes
		for (int i = 0; i < NUM_SWITCH_NODES; i++)
		{
			// loop through outgoing ports to memory nodes
			printf("* Switch node with ID %d at time %d\n", switch_nodes[i].id, global_time);

			for (int j = 0; j < SW_NUM_TOP_PORTS; j++)
			{
				printf("** Top port %d\n", j);
				Packet curr_packet_tx = pop_packet((&(switch_nodes[i].top_ports[j])), TX, 0);
				Packet curr_packet_rx = pop_packet((&(switch_nodes[i].top_ports[j])), RX, 0);
				if ((curr_packet_tx.flag != ERROR) && (curr_packet_tx.time <= global_time))
				{
					// need to act on this packet
					if ((curr_packet_tx.dst >= compute_node_min_id) && (curr_packet_tx.dst <= compute_node_max_id))
					{
						printf(ANSI_COLOR_BLUE "Moving packet with ID %d to compute node with ID %d" ANSI_COLOR_RESET "\n", curr_packet_tx.id, curr_packet_tx.dst);
						curr_packet_tx.time = global_time + GLOBAL_TIME_INCR; // TODO custom time
						// place packet into compute node
						push_packet((&(compute_nodes[j].bot_ports[i])), RX, curr_packet_tx);
						// write to log file
						char hex_addr[11];
						char hex_data[11];
						snprintf(hex_addr, sizeof(hex_addr), "0x%08X", curr_packet_tx.data.addr);
						snprintf(hex_data, sizeof(hex_data), "0x%08X", curr_packet_tx.data.data);
						fprintf(output_file, "%d,%d,%d,%d,%d,%s,%s\n", global_time, curr_packet_tx.id, curr_packet_tx.flag, curr_packet_tx.src, curr_packet_tx.dst, hex_addr, hex_data);
					}
					else
					{
						printf("Packet with ID %d has invalid destination\n", curr_packet_tx.id);
					}
					// remove packet from switch top output port
					pop_packet((&(switch_nodes[i].top_ports[j])), TX, 1);
				}

				if ((curr_packet_rx.flag != ERROR) && (curr_packet_rx.time <= global_time))
				{
					// need to act on this packet
					// if ((curr_packet_rx.dst >= memory_node_min_id) && (curr_packet_rx.dst <= memory_node_max_id))
					// {
						printf(ANSI_COLOR_BLUE "Moving packet with ID %d to output queue" ANSI_COLOR_RESET "\n", curr_packet_rx.id);
						curr_packet_rx.time = global_time + GLOBAL_TIME_INCR; // TODO custom time
						// place node into switch bottom output port corresponding to address
						uint32_t correct_memory_node = ((curr_packet_rx.data.addr >> 3) / 64);
						curr_packet_rx.dst = correct_memory_node + memory_node_min_id;
						push_packet((&(switch_nodes[i].bot_ports[correct_memory_node])), TX, curr_packet_rx);
					// }
					// else
					// {
					// 	printf("Packet with ID %d has invalid destination\n", curr_packet_rx.id);
					// }
					// remove packet from switch top input port
					pop_packet((&(switch_nodes[i].top_ports[j])), RX, 1);
				}
			}

			for (int j = 0; j < SW_NUM_BOT_PORTS; j++)
			{
				printf("** Bottom port %d\n", j);
				Packet curr_packet_tx = pop_packet((&(switch_nodes[i].bot_ports[j])), TX, 0);
				Packet curr_packet_rx = pop_packet((&(switch_nodes[i].bot_ports[j])), RX, 0);
				if ((curr_packet_tx.flag != ERROR) && (curr_packet_tx.time <= global_time))
				{
					// need to act on this packet
					// if ((curr_packet_tx.dst >= memory_node_min_id) && (curr_packet_tx.dst <= memory_node_max_id))
					// {
						printf(ANSI_COLOR_BLUE "Moving packet with ID %d to memory node with ID %d" ANSI_COLOR_RESET "\n", curr_packet_tx.id, curr_packet_tx.dst);
						curr_packet_tx.time = global_time + GLOBAL_TIME_INCR; // TODO custom time
						// place packet into memory node
						push_packet((&(memory_nodes[j].top_ports[i])), RX, curr_packet_tx);
						// write to log file
						char hex_addr[11];
						char hex_data[11];
						snprintf(hex_addr, sizeof(hex_addr), "0x%08X", curr_packet_tx.data.addr);
						snprintf(hex_data, sizeof(hex_data), "0x%08X", curr_packet_tx.data.data);
						fprintf(output_file, "%d,%d,%d,%d,%d,%s,%s\n", global_time, curr_packet_tx.id, curr_packet_tx.flag, curr_packet_tx.src, curr_packet_tx.dst, hex_addr, hex_data);
					// }
					// else
					// {
					// 	printf("Packet with ID %d has invalid destination\n", curr_packet_tx.id);
					// }
					// remove packet from switch bottom output port
					pop_packet((&(switch_nodes[i].bot_ports[j])), TX, 1);
				}

				if ((curr_packet_rx.flag != ERROR) && (curr_packet_rx.time <= global_time))
				{
					// need to act on this packet
					if ((curr_packet_rx.dst >= compute_node_min_id) && (curr_packet_rx.dst <= compute_node_max_id))
					{
						printf(ANSI_COLOR_BLUE "Moving packet with ID %d to output queue" ANSI_COLOR_RESET "\n", curr_packet_rx.id);
						curr_packet_rx.time = global_time + GLOBAL_TIME_INCR; // TODO custom time
						// place packet into switch top output port
						uint32_t correct_compute_node = curr_packet_rx.dst - compute_node_min_id;
						push_packet((&(switch_nodes[i].top_ports[correct_compute_node])), TX, curr_packet_rx);
					}
					else
					{
						printf(ANSI_COLOR_RED "Packet with ID %d has invalid destination" ANSI_COLOR_RESET "\n", curr_packet_rx.id);
					}
					// remove packet from switch bottom input port
					pop_packet((&(switch_nodes[i].bot_ports[j])), RX, 1);
				}
			}
		}

		// // outgoing from memory nodes
		for (int i = 0; i < NUM_MEMORY_NODES; i++)
		{
			printf("* Memory node with ID %d at time %d\n", memory_nodes[i].id, global_time);
			// loop through outgoing ports to switch nodes
			for (int j = 0; j < MEM_NUM_TOP_PORTS; j++)
			{
				Packet curr_packet_tx = pop_packet((&(memory_nodes[i].top_ports[j])), TX, 0);
				Packet curr_packet_rx = pop_packet((&(memory_nodes[i].top_ports[j])), RX, 0);
				if ((curr_packet_tx.flag != ERROR) && (curr_packet_tx.time <= global_time))
				{
					// need to act on this packet
					if ((curr_packet_tx.dst >= compute_node_min_id) && (curr_packet_tx.dst <= compute_node_max_id))
					{
						printf(ANSI_COLOR_BLUE "Moving packet with ID %d to switch with ID %d" ANSI_COLOR_RESET "\n", curr_packet_tx.id, switch_nodes[0].id);
						curr_packet_tx.time = global_time + GLOBAL_TIME_INCR; // TODO custom time
						// place packet into switch
						push_packet((&(switch_nodes[0].bot_ports[i])), RX, curr_packet_tx);
					}
					else
					{
						printf(ANSI_COLOR_RED "Packet with ID %d has invalid destination" ANSI_COLOR_RESET "\n", curr_packet_tx.id);
					}
					// remove packet from memory node
					pop_packet((&(memory_nodes[i].top_ports[j])), TX, 1);
				}

				if ((curr_packet_rx.flag != ERROR) && (curr_packet_rx.time <= global_time))
				{
					// need to act on this packet
					// if (curr_packet_rx.dst == memory_nodes[i].id)
					// {
						printf(ANSI_COLOR_GREEN "Packet with ID %d has arrived at memory node with ID %d [0x%08x, 0x%08x]" ANSI_COLOR_RESET "\n", curr_packet_rx.id, memory_nodes[i].id, curr_packet_rx.data.addr, curr_packet_rx.data.data);
						// handle return packet
						Packet return_packet = process_packet(memory_nodes[i], curr_packet_rx, global_id++, global_time, memory_node_min_id);
						if (return_packet.flag != ERROR)
						{
							// place packet in outgoing buffer
							push_packet((&(memory_nodes[i].top_ports[j])), TX, return_packet);
						}
					// }
					// else
					// {
					// 	printf("Packet with ID %d has invalid destination\n", curr_packet_rx.id);
					// }
					// remove packet from memory node
					pop_packet((&(memory_nodes[i].top_ports[j])), RX, 1);
				}
			}
		}
		
		// end of loop
		global_time += GLOBAL_TIME_INCR;

		finished = 1;
		for (int i = 0; i < NUM_COMPUTE_NODES; i++)
		{
			for (int j = 0; j < CMP_NUM_BOT_PORTS; j++)
			{
				finished &= port_empty(compute_nodes[i].bot_ports[j]);
			}
		}
		for (int i = 0; i < NUM_SWITCH_NODES; i++)
		{
			for (int j = 0; j < SW_NUM_TOP_PORTS; j++)
			{
				finished &= port_empty(switch_nodes[i].top_ports[j]);
			}
			for (int j = 0; j < SW_NUM_BOT_PORTS; j++)
			{
				finished &= port_empty(switch_nodes[i].bot_ports[j]);
			}
		}
		for (int i = 0; i < NUM_MEMORY_NODES; i++)
		{
			for (int j = 0; j <MEM_NUM_TOP_PORTS; j++)
			{
				finished &= port_empty(memory_nodes[i].top_ports[j]);
			}
		}
	} while (finished == 0);

	fclose(output_file);
	return EXIT_SUCCESS;
}