/**************************************************************************
 *  copyright            : (C) 2004-2006 by Petr Schwarz & Pavel Matejka  *
 *                                        UPGM,FIT,VUT,Brno               *
 *  email                : {schwarzp,matejkap}@fit.vutbr.cz               *
 **************************************************************************
 *                                                                        *
 *  This program is free software; you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation; either version 2 of the License, or     *
 *  (at your option) any later version.                                   *
 *                                                                        *
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <search.h>
#include <set>
#include "fsm.h"

// definition of INFINITY
char fsm_infinity_bytes[] = {0x00, 0x00, 0x80, 0x7F};

FSM::FSM() :
	isTransducer(false),
	manageBWArcs(0),
	startNodeId(0),
	nNodes(0),
	nArcs(0),
	semiring(SEMIRING_ACCEP_TROPICAL),
	failureLabel(0),
	firstLin(0),
	lastLin(0),
	startNode(0),
	nodeIndex(0),
	maxNodeId(0)
{
	nodeCache.SetPageSize(sizeof(NODE), DEFAULT_NODE_CACHE);
	arcCache.SetPageSize(sizeof(ARC), DEFAULT_ARC_CACHE);
}

FSM::~FSM()
{
	Erase();
}

void FSM::Erase()
{
	DestroyNodeIndex();

	NODE *node = firstLin;
	while(node)
	{
		ARC *arc = node->fwArcsFirst;
		while(arc)
		{
			ARC *tmpArc = arc;
			arc = arc->nextFW;
			arcCache.Free(tmpArc);
		}
		NODE *tmpNode = node;
		node = node->nextLin;
		nodeCache.Free(tmpNode);
	}

	startNodeId = 0;
	nNodes = 0;
	nArcs = 0;
	semiring = SEMIRING_ACCEP_TROPICAL;
	firstLin = 0;
	lastLin = 0;
	startNode = 0;
	nodeIndex = 0;
	maxNodeId = 0;
	isTransducer = false;
	failureLabel = 0;
	manageBWArcs = 0;
}

void FSM::VerifySemiring()
{
	isTransducer = false;
	NODE *node = firstLin;
	while(node)
	{
		ARC *arc = node->fwArcsFirst;
		while(arc)
		{
			if(arc->labelFrom != arc->labelTo)
				isTransducer = true;
			arc = arc->nextFW;
		}
		node = node->nextLin;
	}

	if(isTransducer)
	{
		switch(semiring)
		{
			case SEMIRING_ACCEP_TROPICAL:
				semiring = SEMIRING_TRANS_TROPICAL;
				break;
			case SEMIRING_ACCEP_LOG:
				semiring = SEMIRING_TRANS_LOG;
				break;
		}
	}
}

NODE *FSM::AddNode(unsigned int id)
{
	NODE *node = (NODE *)nodeCache.Alloc();
	if(!node)
	{
		fprintf(stderr, "ERROR: FSM::AddNode - memory allocation error");
		exit(1);
	}

	if(id == NODE_MAXID)
	{
		if(nNodes != 0)
			maxNodeId++;
		node->id = maxNodeId;
	}
	else
	{
		if(id > maxNodeId)
			maxNodeId = id;
		node->id = id;
	}
	
	node->flag = 0;
	node->potential = 0.0f;
	node->nFwArcs = 0;
	node->nBwArcs = 0;
	node->fwArcsFirst = 0;
	node->bwArcsFirst = 0;
	node->fwArcsLast = 0;
	node->bwArcsLast = 0;
	node->nextLin = 0;
	node->nextTmp = 0;

	SetNodeTermWeight(node, FSM_INFINITY);
	
	if(!lastLin)
	{
		firstLin = node;
		lastLin = node;
		startNode = node;
	}
	else
	{
		lastLin->nextLin = node;
		lastLin = node;
	}
	
	//printf("node id %u %u\n", node->id, id);
	nNodes++;
	
	return node;
}

NODE *FSM::GetNode(unsigned int id)
{
	if(!nodeIndex)
		CreateNodeIndex();
	
	assert(id <= maxNodeId);
	
	return nodeIndex[id].node;
}

NODE *FSM::GetNextNodeIS(NODE *node, unsigned int isymbol)
{
	struct ARC *arc = node->fwArcsFirst;
	while(arc)
	{
		if(arc->labelFrom == isymbol)
			break;
		arc = arc->nextFW;
	}
	if(!arc)
		return 0;
	return arc->end;
}

NODE *FSM::GetNextNodeOS(NODE *node, unsigned int osymbol)
{
	struct ARC *arc = node->fwArcsFirst;
	while(arc)
	{
		if(arc->labelTo == osymbol)
			break;
		arc = arc->nextFW;
	}
	if(!arc)
		return 0;
	return arc->end;
}

void FSM::SetNodeTermWeight(NODE *node, float weight)
{
	assert(node);

	node->termWeight = weight;

	if(node->termWeight == FSM_INFINITY)
		node->flag &= (~(unsigned int)NF_TERMINAL);
	else
		node->flag |= (unsigned int)NF_TERMINAL;
}

ARC *FSM::GetNextArcIS(NODE *from, NODE *to, unsigned int isymbol)
{
	struct ARC *arc = from->fwArcsFirst;
	while(arc)
	{
		if(arc->labelFrom == isymbol && (to == arc->end || to == 0))
			break;
		arc = arc->nextFW;
	}
	if(!arc)
		return 0;
	return arc;
}

ARC *FSM::GetNextArcOS(NODE *from, NODE *to, unsigned int osymbol)
{
	struct ARC *arc = from->fwArcsFirst;
	while(arc)
	{
		if(arc->labelTo == osymbol && (to == arc->end || to == 0))
			break;
		arc = arc->nextFW;
	}
	if(!arc)
		return 0;
	return arc;
}


ARC *FSM::AddArc(NODE *node1, NODE *node2)
{
	assert(node1);
	assert(node2);

	ARC *arc = (ARC *)arcCache.Alloc();

	arc->start = node1;
	arc->end = node2;
	arc->weight = 0.0f;
	arc->nextFW = 0;
	arc->nextBW = 0;
	arc->labelFrom = 0;
	arc->labelTo = 0;
	
	if(node1->fwArcsLast)
		node1->fwArcsLast->nextFW = arc;
	else
		node1->fwArcsFirst = arc;

	node1->fwArcsLast = arc;
	node1->nFwArcs++;

	if(manageBWArcs)
	{
		if(node2->bwArcsLast)
			node2->bwArcsLast->nextBW = arc;
		else
			node2->bwArcsFirst = arc;

		node2->bwArcsLast = arc;
		node2->nBwArcs++;
	}

	nArcs++;
	
	return arc;
}

void FSM::AddBWArc(NODE *node, ARC *arc)
{
	assert(node);
	assert(arc);

	if(node->bwArcsLast)
		node->bwArcsLast->nextBW = arc;
	else
		node->bwArcsFirst = arc;

	node->bwArcsLast = arc;
	node->nBwArcs++;
}

void FSM::NullBWArcs()
{
	NODE *node = firstLin;
	while(node)
	{
		ARC *arc = node->fwArcsFirst;
		while(arc)
		{
			arc->nextBW = 0;
			arc = arc->nextFW;
		}
		node->bwArcsFirst = 0;
		node->bwArcsLast = 0;
		node->nBwArcs = 0;
		node = node->nextLin;
	}
}

void FSM::CreateBWArcs()
{
	NullBWArcs();
	NODE *node = firstLin;
	while(node)
	{
		ARC *arc = node->fwArcsFirst;
		while(arc)
		{
			AddBWArc(arc->end, arc);
			arc = arc->nextFW;
		}
		node = node->nextLin;
	}
}

void FSM::CopyArcContent(ARC *from, ARC *to)
{
	to->labelFrom = from->labelFrom;
	to->labelTo = from->labelTo;
	to->weight = from->weight;
}

unsigned int FSM::GetMaxLabelLimited(unsigned int max, unsigned int labelType)
{
	assert(labelType == LABEL_INPUT || labelType == LABEL_OUTPUT);

	unsigned int maxLabel = 0;

	NODE *node = firstLin;
	while(node)
	{
		ARC *arc = node->fwArcsFirst;
		while(arc)
		{
			unsigned int label = (labelType == LABEL_INPUT ? arc->labelFrom : arc->labelTo);
			if(label <= max && label > maxLabel)
				maxLabel = label;

			arc = arc->nextFW;
		}
		node = node->nextLin;
	}
	return maxLabel;
}

void FSM::ShiftLabel(unsigned int minLabel, unsigned int maxLabel, long long shift, unsigned int labelType)
{
	assert(labelType == LABEL_INPUT || labelType == LABEL_OUTPUT);

	NODE *node = firstLin;
	while(node)
	{
		ARC *arc = node->fwArcsFirst;
		while(arc)
		{
			unsigned int label = (labelType == LABEL_INPUT ? arc->labelFrom : arc->labelTo);
			if(label >= minLabel && label <= maxLabel)
				label = (unsigned int)((long long)label + shift);

			if(labelType == LABEL_INPUT)
				arc->labelFrom = label;
			else
				arc->labelTo = label;

			arc = arc->nextFW;
		}
		node = node->nextLin;
	}
}

void FSM::CreateNodeIndex()
{
	if(nodeIndex)
		return;

	//printf(">> CreateNodeIndex\n");


	unsigned int nIndexValues = maxNodeId + 1;
	nodeIndex = new NODE_INDEX [nIndexValues];
	
	NODE *node = firstLin;
	while(node)
	{
//		printf("|| %d\n", node->id);
		nodeIndex[node->id].node = node;
		nodeIndex[node->id].value = 0;
		node = node->nextLin; 
	}
	
	//printf(">> CreateNodeIndex ends\n");
}
	
void FSM::NullIndexValues()
{
	if(nodeIndex)
		CreateNodeIndex();
		
	unsigned int i;
	for(i = 0; i < maxNodeId + 1; i++)
		nodeIndex[i].value = 0;
}

void FSM::DestroyNodeIndex()
{
	if(nodeIndex)
	{
		delete [] nodeIndex;
		nodeIndex = 0;
	}
}

void FSM::GetSignature(FILE *fp, char *buff, int maxLen)
{
	int i = 0;
	int ch;
	do
	{
		ch = fgetc(fp);
		if(ch != 0x0A)
		{
			buff[i] = ch;
			i++;
		}
	} while(ch != 0x0A && i < maxLen - 1);
	buff[i] = '\0';
}

void FSM::LoadBinAtt(char *file)
{
        Erase();

	FILE *fp;
	fp = fopen(file, "rb");
	if(!fp)
	{
		fprintf(stderr, "ERROR: FSM::LoadBinAtt - can not open file: %s\n", file);
		exit(1);
	}

	// Read header signature. The signature has variable length terminated by 0x0A 
	char signature[21];
	GetSignature(fp, signature, 21);
	//printf(">> %s\n", signature);
	
	// in case of failure class, there is a special header "FSM/failure" followed by
	// a failure label (unsigned int) and by the common header (FSM ...)
	if(strcmp(signature, "FSM/failure") == 0)
	{
		unsigned int l;
		if(fread(&l, sizeof(unsigned int), 1, fp) != 1)
		{
			fprintf(stderr, "ERROR: FSM::LoadBinAtt - corrupted file %s\n", file);
			exit(1);
		}
		failureLabel = l;
		GetSignature(fp, signature, 21);
	}
	
	if(strcmp(signature, "FSM") != 0)
	{
		fprintf(stderr, "ERROR: FSM::LoadBinAtt - unsupported FSM format %s in file %s\n", signature, file);
		exit(1);
	}
		
	// Read the header
	struct ATT_BIN_HEADER header;
	
	if(fread(&header, sizeof(header), 1, fp) != 1)
	{
		fprintf(stderr, "ERROR: FSM::LoadBinAtt - corrupted file %s\n", file);
		exit(1);
	}

	//printf("class     %u\n", header.fsmClass);
	//printf("semiring  %u\n", header.semiring);
	//printf("nodes     %u\n", header.nNodes);
	//printf("startNode %u\n", header.startNode);

	startNodeId = header.startNode;
	semiring = header.semiring;
	nNodes = 0;
	
	// Allocate linear list of nodes
	unsigned int i;
	for(i = 0; i < header.nNodes; i++)
		AddNode();
	
	struct NODE *actNode = firstLin;
	startNode = 0;
	
	// Create index to the list
	CreateNodeIndex();

	// Read all nodes
	struct ATT_BIN_NODE node;
	struct ATT_BIN_ARC arc;
	isTransducer = false;
	
	for(i = 0; i < nNodes; i++)
	{
		if(fread(&node, sizeof(node), 1, fp) != 1)
		{
			fprintf(stderr, "ERROR: FSM::LoadBinAtt - corrupted file %s\n", file);
			exit(1);
		}
		//printf("potential  %f\n", node.potential);
		//printf("termWeight %f\n", node.termWeight);
		//printf("nArcs      %u\n", node.nArcs);

		// fill in the node info
		//actNode->ref1.id = i;
		actNode->potential = node.potential;
		actNode->termWeight = node.termWeight;
		actNode->flag = 0;
		actNode->nFwArcs = 0;
		SetNodeTermWeight(actNode, node.termWeight);

		if(i == header.startNode)
			startNode = actNode;
		//printf("%u\n", actNode->flag);

		actNode->fwArcsFirst = 0;
		actNode->fwArcsLast = 0;
		actNode->bwArcsFirst = 0;
		actNode->bwArcsLast = 0;

		// fill in arcs
		unsigned int j;
		for(j = 0; j < node.nArcs; j++)
		{
			if(fread(&arc, sizeof(arc), 1, fp) != 1)
			{
				fprintf(stderr, "ERROR: FSM::LoadBinAtt - corrupted file %s\n", file);
				exit(1);
			}

			//printf("%u\t%u\t%u\t%u\t%f\n", i, arc.target, arc.labelFrom, arc.labelTo, arc.weight);

			if(arc.target > maxNodeId)
			{
				fprintf(stderr, "ERROR: FSM::LoadBinAtt - invalid destination state, %u, for state %u, arc %u, file %s\n", arc.target, i, j, file);
				exit(1);
			}
			ARC *arc1 = AddArc(actNode, nodeIndex[arc.target].node);

			arc1->labelFrom = arc.labelFrom;
			arc1->labelTo = arc.labelTo;
			arc1->weight = arc.weight;
			//arc1->start = actNode;

			//printf("%d\n", arc.target);
			if(arc1->labelFrom != arc1->labelTo)
				isTransducer = true;
			
			assert(arc1->end);
		}

		actNode = actNode->nextLin;
	}
	fclose(fp);
	
	// check if the start node was found
	if(!startNode)
	{
		fprintf(stderr, "ERROR: FSM::LoadBinAtt - invalid start node %d\n", header.startNode);
		exit(1);
	}
}

bool FSM::LoadFailureHeader(char *file, unsigned int *label)
{
	FILE *fp;
	fp = fopen(file, "rb");
	if(!fp)
	{
		fprintf(stderr, "ERROR: FSM::LoadBinAtt - can not open file: %s\n", file);
		exit(1);
	}
	
	// Read header signature. The signature has variable length terminated by 0x0A 
	char signature[21];
	GetSignature(fp, signature, 21);
	
	// in case of failure class, there is a special header "FSM/failure" followed by 
	// a failure label (unsigned int) and by the common header (FSM ...)
	if(strcmp(signature, "FSM/failure") == 0)
	{
		if(fread(label, sizeof(unsigned int), 1, fp) != 1)
		{
			fprintf(stderr, "ERROR: FSM::LoadBinAtt - corrupted file %s\n", file);
			exit(1);
		}
		fclose(fp);
		return true;
	}
	fclose(fp);
	return false;
}

void FSM::RenumberNodeIds()
{
//	SaveBinAtt("A.fsm");
	NODE *node = firstLin;
	int i = 0;
	while(node)
	{
		node->id = i;
		node = node->nextLin;
		i++;
	}
	DestroyNodeIndex();
//	SaveBinAtt("B.fsm");
}

// Compares arc pointers according target node ID
int FSM::CmpArcToNodeId(const void *a, const void *b)
{
	assert(a);
	assert(b); 
		
	ARC *na = *(ARC **)a;
	ARC *nb = *(ARC **)b;

//	printf("%u %p %p %u %u\n", na->labelFrom, a, b, na->end->ref1.id, nb->end->ref1.id);
	
	assert(na->end);
	assert(nb->end);
	
	if(na->end == nb->end)
		return 0;
	if(na->end < nb->end)
		return -1;
	return 1;
}

// Compares arc pointers according input label
int FSM::CmpArcLabelFrom(const void *a, const void *b)
{
	assert(a);
	assert(b); 
		
	ARC *na = *(ARC **)a;
	ARC *nb = *(ARC **)b;

//	printf("%u %p %p %u %u\n", na->labelFrom, a, b, na->end->ref1.id, nb->end->ref1.id);
	
	assert(na->end);
	assert(nb->end);
	
	if(na->labelFrom == nb->labelFrom)
		return 0;
	if(na->labelFrom < nb->labelFrom)
		return -1;
	return 1;
}

// Compares arc pointers according output label
int FSM::CmpArcLabelTo(const void *a, const void *b)
{
	assert(a);
	assert(b); 
		
	ARC *na = *(ARC **)a;
	ARC *nb = *(ARC **)b;

//	printf("%u %p %p %u %u\n", na->labelFrom, a, b, na->end->ref1.id, nb->end->ref1.id);
	
	assert(na->end);
	assert(nb->end);
	
	if(na->labelTo == nb->labelTo)
		return 0;
	if(na->labelTo < nb->labelTo)
		return -1;
	return 1;
}

// Sorts arcs according target node id
void FSM::SortArcs(ARC_CMP_FUNC fce)
{
	NODE *node = firstLin;
	while(node)
	{
                SortArcs(node, fce);
		node = node->nextLin;
//		puts("-------------------");
	}
}

void FSM::SortArcs(NODE *node, ARC_CMP_FUNC fce)
{
//	printf("%u\n", node->ref1.id);

	ARC **parc = new ARC *[node->nFwArcs];
	if(!parc)
	{
		fprintf(stderr, "ERROR: FSM::SortArcs - Memory allocation error\n");
		exit(1);
	}

	if(node->nFwArcs > 1)
	{
		// copy arc pointers to the array
		ARC *arc = node->fwArcsFirst;
		int i;
		for(i = 0; i < node->nFwArcs; i++)
		{
			assert(arc);
			parc[i] = arc;
			arc = arc->nextFW;
		}

		// sort arcs
		qsort(parc, node->nFwArcs, sizeof(ARC *), fce);

		// create new linear array of arcs
		node->fwArcsFirst = parc[0];
		node->fwArcsLast = parc[node->nFwArcs - 1];

		for(i = 0; i < node->nFwArcs - 1; i++)
		{
			parc[i]->nextFW = parc[i + 1];

//			printf("%u\n", parc[i]->end->ref1.id);
		}

		parc[node->nFwArcs - 1]->nextFW = 0;
	}

	delete [] parc;
}

void FSM::SaveBinAtt(char *file)
{
	VerifySemiring();
	RenumberNodeIds();

	FILE *fp;
	fp = fopen(file, "wb");
	if(!fp)
	{
		fprintf(stderr, "ERROR: FSM::SaveBinAtt - can not create file: %s\n", file);
		exit(1);
	}

	struct ATT_BIN_HEADER header;
	struct ATT_BIN_NODE node;
	struct ATT_BIN_ARC arc;

	// write signature
	char signature[5];
	strcpy(signature, "FSM ");
	signature[3] = 0x0A;

	if(fwrite(signature, strlen(signature), 1, fp) != 1)
	{
		fprintf(stderr, "ERROR: FSM::SaveBinAtt - unable to save file %s\n", file);
		exit(1);
	}

	// write a dummy header
	memset(&header, 0, sizeof(header));
	if(fwrite(&header, sizeof(header), 1, fp) != 1)
	{
		fprintf(stderr, "ERROR: FSM::SaveBinAtt - unable to save file %s\n", file);
		exit(1);
	}

	// write nodes
	NODE *actNode = firstLin;
	int i = 0;
	while(actNode)
	{
//		printf("%u\n", actNode->ref1.id);
		node.potential = actNode->potential;
		node.termWeight = actNode->termWeight;
		node.nArcs = actNode->nFwArcs;
		if(actNode == startNode)
			header.startNode = actNode->id;

		if(fwrite(&node, sizeof(node), 1, fp) != 1)
		{
			fprintf(stderr, "ERROR: FSM::SaveBinAtt - corrupted file %s\n", file);
			exit(1);
		}

		ARC *arc1 = actNode->fwArcsFirst;
		int j;
		for(j = 0; j < actNode->nFwArcs; j++)
		{
			arc.labelFrom = arc1->labelFrom;
			arc.labelTo = arc1->labelTo;
			//printf("%p\n", arc1->end);
			arc.target = arc1->end->id;
			arc.weight =  arc1->weight;

			//printf("%u\t%u\t%u\t%u\t%f\n", i, arc.target, arc.labelFrom, arc.labelTo, arc.weight);

			if(fwrite(&arc, sizeof(arc), 1, fp) != 1)
			{
				fprintf(stderr, "ERROR: FSM::LoadBinAtt - corrupted file %s\n", file);
				exit(1);
			}
			arc1 = arc1->nextFW;
		}

		actNode = actNode->nextLin;
		i++;
	}

	// write filled in header
	header.fsmClass = CLASS_BASIC;
	header.semiring = semiring;
	header.nNodes = nNodes;

	fseek(fp, strlen(signature), SEEK_SET);
	if(fwrite(&header, sizeof(header), 1, fp) != 1)
	{
		fprintf(stderr, "ERROR: FSM::SaveBinAtt - unable to save file %s\n", file);
		exit(1);
	}

	fclose(fp);
}

void FSM::Semiring2Str(unsigned int semiring, char *str)
{
	assert(semiring == SEMIRING_ACCEP_TROPICAL || semiring == SEMIRING_TRANS_TROPICAL ||
	       semiring == SEMIRING_ACCEP_LOG || semiring == SEMIRING_TRANS_LOG);

	switch(semiring)
	{
		case SEMIRING_ACCEP_TROPICAL:
		case SEMIRING_TRANS_TROPICAL:
			strcpy(str, "tropical");
			break;
		case SEMIRING_ACCEP_LOG:
		case SEMIRING_TRANS_LOG:
			strcpy(str, "log");
			break;
	}
}

void FSM::Bool2Str(bool v, char *str)
{
	if(v)
		strcpy(str, "true");
	else
		strcpy(str, "false");
}

//-----------------------------------------------------------------------------------------------------

NODE_STACK::NODE_STACK()
{
	nNodes = 0;
	first = 0;
	last = 0;
}

void NODE_STACK::Add(struct NODE *node)
{
	assert(node);
	
	node->nextTmp = 0;
	if(last)
		last->nextTmp = node;
	else
		first = node;

	last = node; 
	nNodes++;
}

struct NODE *NODE_STACK::Get()
{
	if(!nNodes)
		return 0;
	
	NODE *node = first;
	
	first = first->nextTmp;
	if(!first)
		last = 0;
	nNodes--;
	
	return node;
}

//-----------------------------------------------------------------------------------------------------

struct ltComposeIndex
{
	bool operator()(const COMPOSE_INDEX *a, const COMPOSE_INDEX *b) const
	{
		if(a->id1 < b->id1)
			return true;
		else if(a->id1 > b->id1)
			return false;
		else if(a->id2 < b->id2)
			return true;
			
		return false;
	}
};

FSM *FSM_ALGO::compose(FSM *A, FSM *B)
{	
	FSM *C = new FSM;
	
	// sort arcs acording targets
	A->SortArcs(FSM::CmpArcLabelTo);
	B->SortArcs(FSM::CmpArcLabelFrom);
	
	// initialize the algorithm
	NODE_STACK stack;
	NODE *node = C->AddNode();
	node->id1 = A->startNodeId;
	node->id2 = B->startNodeId;
	
	stack.Add(node);  // this node should be expanded
	
	// initialize stack of output nodes
	std::set<const struct COMPOSE_INDEX *, ltComposeIndex> composedNodes;
	std::set<const struct COMPOSE_INDEX *, ltComposeIndex>::iterator it;
	struct COMPOSE_INDEX ref;
	
	// expand each node in stack
	while(!stack.IsEmpty())
	{
		//printf("A: %u\n", stack.GetNodesNum());
		node = stack.Get();
		//printf("B: %u\n", stack.GetNodesNum());
		
		NODE *nodeA = A->GetNode(node->id1);
		NODE *nodeB = B->GetNode(node->id2);
		
		//printf("id: %6u %6u ", nodeA->id, nodeB->id);
		unsigned int nExpNodes = 0;
		
		ARC *arcA = nodeA->fwArcsFirst;
		ARC *arcB = nodeB->fwArcsFirst; 
		
		while(arcA && arcA->labelTo == 0) // eps 
		{
			//puts("eps A");
			
			NODE *newNode;
			ref.id1 = arcA->end->id;
			ref.id2 = nodeB->id;
			it = composedNodes.find(&ref);
			if(it != composedNodes.end())
			{
				newNode = (*it)->node;
			}
			else
			{
				newNode = C->AddNode();
				nExpNodes++;
			}
				
			newNode->id1 = arcA->end->id;
			newNode->id2 = nodeB->id;
			
			ARC *newArc = C->AddArc(node, newNode);
			newArc->labelFrom = arcA->labelFrom;
			newArc->labelTo = arcA->labelTo;
			newArc->weight = arcA->weight;
			
			if(it == composedNodes.end())
			{
				stack.Add(newNode);
			
				struct COMPOSE_INDEX *idxEntry = new struct COMPOSE_INDEX;
				idxEntry->id1 = newNode->id1;
				idxEntry->id2 = newNode->id2;
				idxEntry->id = newNode->id;
				idxEntry->node = newNode;
				composedNodes.insert(idxEntry);
			}
			
			//AddToTmpList(NODE *root, NODE *node)
			arcA = arcA->nextFW;
			//puts("eps A ends");
		}
		
		while(arcB && arcB->labelFrom == 0) // eps
		{
			//puts("eps B");

			NODE *newNode;
			ref.id1 = nodeA->id;
			ref.id2 = arcB->end->id;
			it = composedNodes.find(&ref);
			if(it != composedNodes.end())
			{
				newNode = (*it)->node;
			}
			else
			{
				newNode = C->AddNode();
				nExpNodes++;
			}
			
			newNode->id1 = nodeA->id;
			newNode->id2 = arcB->end->id;
			
			ARC *newArc = C->AddArc(node, newNode);
			newArc->labelFrom = arcB->labelFrom;
			newArc->labelTo = arcB->labelTo;
			newArc->weight = arcB->weight;

			if(it == composedNodes.end())
			{
				stack.Add(newNode);
			
				struct COMPOSE_INDEX *idxEntry = new struct COMPOSE_INDEX;
				idxEntry->id1 = newNode->id1;
				idxEntry->id2 = newNode->id2;
				idxEntry->id = newNode->id;
				idxEntry->node = newNode;
				composedNodes.insert(idxEntry);
			}
			
			arcB = arcB->nextFW; 
			//puts("eps B ends");	
		}

		while(arcA && arcB)
		{
			if(arcA->labelTo == arcB->labelFrom)
			{
//				puts("aaaa");
				NODE *newNode;
				ref.id1 = arcA->end->id;
				ref.id2 = arcB->end->id;
				it = composedNodes.find(&ref);
				if(it != composedNodes.end())
				{
					newNode = (*it)->node;
				}
				else
				{
					newNode = C->AddNode();
					nExpNodes++;
				}

				newNode->id1 = arcA->end->id;
				newNode->id2 = arcB->end->id;
				
				ARC *newArc = C->AddArc(node, newNode);
				newArc->labelFrom = arcA->labelTo;
				newArc->labelTo = arcB->labelFrom;
				newArc->weight = arcA->weight + arcB->weight;
				
				if(it == composedNodes.end())
				{
						stack.Add(newNode);
				
					struct COMPOSE_INDEX *idxEntry = new struct COMPOSE_INDEX;
					idxEntry->id1 = newNode->id1;
					idxEntry->id2 = newNode->id2;
					idxEntry->id = newNode->id;
					idxEntry->node = newNode;
					composedNodes.insert(idxEntry);
				}
				
				arcA = arcA->nextFW;
//				puts("aaaa ends");
			}
			else if(arcA->labelTo < arcB->labelFrom)
			{
				arcA = arcA->nextFW;
			}
			else
			{
				arcB = arcB->nextFW;
			}
		}
		
		//printf("nc:%6u ns:%6u ex: %u\n", C->GetNodesNum(), stack.GetNodesNum(), nExpNodes);
		//printf("-------------\n");
	}
	
	return C;
}

void FSM::RemoveArcs(unsigned int minLabel, unsigned int maxLabel, unsigned int labelType)
{
	assert(labelType == LABEL_INPUT || labelType == LABEL_OUTPUT);

	NODE *node = firstLin;
	while(node)
	{
		ARC *arc = node->fwArcsFirst;
		ARC *prevArc = 0;
		ARC *nextArc;
		while(arc)
		{
			if((labelType == LABEL_INPUT && arc->labelFrom >= minLabel && arc->labelFrom <= maxLabel) ||
			    (labelType == LABEL_OUTPUT && arc->labelTo >= minLabel && arc->labelTo <= maxLabel))
			{
				nextArc = arc->nextFW;

				if(prevArc)
					prevArc->nextFW = nextArc;
				else
					node->fwArcsFirst = arc->nextFW;

				if(node->fwArcsLast == arc)
					node->fwArcsLast = prevArc;

				if(manageBWArcs)
					RemoveBWArc(arc->end, arc);

				node->nFwArcs--;
				nArcs--;

				arcCache.Free(arc);

				arc = nextArc;
			}
			else
			{
				prevArc = arc;
				arc = arc->nextFW;
			}
		}
		node = node->nextLin;
	}
}

void FSM::RemoveBWArc(NODE *node, ARC *rmArc)
{
	assert(node);
	assert(rmArc);

	ARC *arc = node->bwArcsFirst;
	ARC *prevArc = 0;
	while(arc)
	{
		if(arc == rmArc)
		{
			if(prevArc)
				prevArc->nextBW = arc->nextBW;
			else
				node->bwArcsFirst = arc->nextBW;

			if(node->bwArcsLast == arc)
				node->bwArcsLast = prevArc;

			node->nBwArcs--;

			arc = arc->nextBW;
		}
		else
		{
			prevArc = arc;
			arc = arc->nextBW;
		}
	}
}

unsigned int FSM::CountIncomingArcs()
{
	unsigned int fullSum = 0;

	// Null number of backward arcs for each node
	NODE *node = firstLin;
	while(node)
	{
		node->nBwArcs = 0;
		node = node->nextLin;
	}

	// Count backward arcs using forward arcs (for case the backward arcs are not maintained)
	node = firstLin;
	while(node)
	{
		ARC *arc = node->fwArcsFirst;
		while(arc)
		{
			arc->end->nBwArcs++;
			fullSum++;
			arc = arc->nextFW;
		}
		node = node->nextLin;
	}
	return fullSum;
}

void FSM::RemoveFreeNodes()
{
	unsigned int arcsSum = CountIncomingArcs();
	unsigned int prevArcsSum = 0;

	while(nArcs != prevArcsSum)
	{
		NODE *node = firstLin;
		NODE *prevNode = 0;
		while(node)
		{
			if(node->nBwArcs == 0 && node != startNode)
			{
				// remove backward arcs
				ARC *arc;
				if(manageBWArcs)
				{
					arc = node->fwArcsFirst;
					while(arc)
					{
						RemoveBWArc(arc->end, arc);
						arc = arc->nextFW;
					}
				}

				// remove forward arcs
				arc = node->fwArcsFirst;
				while(arc)
				{
					ARC *tmpArc = arc->nextFW;
					arcCache.Free(arc);
					node->nFwArcs--;
					nArcs--;
					arc = tmpArc;
				}
				node->fwArcsFirst = 0;
				node->fwArcsLast = 0;

				// remove node
				NODE *nextNode = node->nextLin;
				if(prevNode)
					prevNode->nextLin = nextNode;
				else
					firstLin = nextNode;

				if(node == lastLin)
					lastLin = prevNode;

				nodeCache.Free(node);
				nNodes--;

				node = nextNode;
			}
			else
			{
				prevNode = node;
				node = node->nextLin;
			}
		}
		prevArcsSum = arcsSum;
		arcsSum = CountIncomingArcs();
		//printf("%d %d\n", arcsSum, prevArcsSum);
	}
}

void FSM::Convert2SVite()
{
	// counts incomming arcs for each node -> nBwArcs
	CountIncomingArcs();

	// null word ids and flags for each node
	NODE *node = firstLin;
	while(node)
	{
		node->potential = -1.0f;
		ResetNodeFlag(node, NF_MODEL);
		ResetNodeFlag(node, NF_WORD);
		node = node->nextLin;
	}

	node = firstLin;
	while(node)
	{
		ARC *arc = node->fwArcsFirst;
		while(arc)
		{
			if(manageBWArcs && (arc->labelFrom != 0 || arc->labelTo != 0))
				RemoveBWArc(arc->end, arc);

			if(arc->labelFrom != 0 && arc->labelTo != 0)
			{
				// new model node
				NODE *modelNode = AddNode();
				modelNode->potential = (float)arc->labelFrom;
				SetNodeFlag(modelNode, NF_MODEL);
				// new word node
				NODE *wordNode;
				if(arc->end->nBwArcs == 1)  // if the following node has one incoming arc only, use the node as word node
					wordNode = arc->end;
				else
					wordNode = AddNode();
				wordNode->potential = (float)arc->labelTo;
				SetNodeFlag(wordNode, NF_WORD);
				// link model node -> word node
				ARC *newArc1 = AddArc(modelNode, wordNode);
				newArc1->labelFrom = 0;
				newArc1->labelTo = 0;
				newArc1->weight = 0.0f;
                               	// link word node -> previous target node
				if(wordNode != arc->end)
				{
					ARC *newArc2 = AddArc(wordNode, arc->end);
					newArc2->labelFrom = 0;
					newArc2->labelTo = 0;
					newArc2->weight = 0.0f;
				}
				// link start node -> model node
				arc->end = modelNode;
				arc->labelFrom = 0;
				arc->labelTo = 0;

				if(manageBWArcs)
					AddBWArc(modelNode, arc);
			}
			else if(arc->labelFrom != 0 && arc->labelTo == 0)
			{
				// model node
				if(arc->end->nBwArcs == 1)  // if the following node has one incoming arc only, use the node
				{
					arc->end->potential = (float)arc->labelFrom;
					SetNodeFlag(arc->end, NF_MODEL);
					arc->labelFrom = 0;
					arc->labelTo = 0;

					if(manageBWArcs)
						AddBWArc(arc->end, arc);
				}
				else
				{
					NODE *modelNode = AddNode();
					modelNode->potential = (float)arc->labelFrom;
					SetNodeFlag(modelNode, NF_MODEL);
					// link model node -> previous target node
					ARC *newArc = AddArc(modelNode, arc->end);
					newArc->labelFrom = 0;
					newArc->labelTo = 0;
					newArc->weight = 0.0f;
					// link start node -> model node
					arc->end = modelNode;
					arc->labelFrom = 0;
					arc->labelTo = 0;

					if(manageBWArcs)
						AddBWArc(modelNode, arc);
				}
			}
			else if(arc->labelFrom == 0 && arc->labelTo != 0)
			{
				if(arc->end->nBwArcs == 1)  // if the following node has one incoming arc only, use the node
				{
					arc->end->potential = (float)arc->labelTo;
					SetNodeFlag(arc->end, NF_WORD);
					arc->labelFrom = 0;
					arc->labelTo = 0;

					if(manageBWArcs)
						AddBWArc(arc->end, arc);
				}
				else
				{
					// new model node
					NODE *wordNode = AddNode();
					wordNode->potential = (float)arc->labelTo;
					SetNodeFlag(wordNode, NF_WORD);
					// link model node -> previous target node
					ARC *newArc = AddArc(wordNode, arc->end);
					newArc->labelFrom = 0;
					newArc->labelTo = 0;
					newArc->weight = 0.0f;
					// link start node -> word node
					arc->end = wordNode;
					arc->labelFrom = 0;
					arc->labelTo = 0;

					if(manageBWArcs)
						AddBWArc(wordNode, arc);
				}
			}

			arc = arc->nextFW;
		}

		//node->potentila = -1.0f;
		//ResetNodeFlag(node, NF_MODEL);
		//ResetNodeFlag(node, NF_WORD);
		node = node->nextLin;
	}
	RenumberNodeIds();
}

void FSM::ReplaceLabels(unsigned int n, unsigned int *from, unsigned int *to, unsigned int labelType)
{
	assert(labelType == LABEL_INPUT || labelType == LABEL_OUTPUT);

	NODE *node = firstLin;
	while(node)
	{
		ARC *arc = node->fwArcsFirst;
		while(arc)
		{
			unsigned i;
			for(i = 0; i < n; i++)
			{
				if(labelType == LABEL_INPUT && arc->labelFrom == from[i])
					arc->labelFrom = to[i];
				if(labelType == LABEL_OUTPUT && arc->labelTo == from[i])
					arc->labelTo = to[i];
			}

			arc = arc->nextFW;
		}
		node = node->nextLin;
	}
}


