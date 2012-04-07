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

#ifndef _FSM_H

#include <limits.h>
#include <stdio.h>
#include <math.h>
#include "fsmcache.h"

// semirings
#define SEMIRING_ACCEP_TROPICAL        0
#define SEMIRING_TRANS_TROPICAL        1
#define SEMIRING_ACCEP_LOG             5
#define SEMIRING_TRANS_LOG             6

// class types
#define CLASS_BASIC                    2
#define CLASS_INDEXED                  4

// node flags
#define NF_TERMINAL                    1
#define NF_MODEL                       2       // for conversion to the SVite format
#define NF_WORD                        4

//
#define NODE_MAXID                UINT_MAX

// label type
#define LABEL_INPUT                    0
#define LABEL_OUTPUT                   1

#define DEFAULT_NODE_CACHE          1000
#define DEFAULT_ARC_CACHE           1000

//#ifndef INFINITY
extern char fsm_infinity_bytes[];
#define FSM_INFINITY *((float *)fsm_infinity_bytes) 
//#endif

struct ARC
{
	struct NODE *start;
	struct NODE *end;
	unsigned int labelFrom;
	unsigned int labelTo;
	float weight;
	struct ARC *nextFW;
	struct ARC *nextBW;
};

struct NODE
{
	unsigned int id;
	int flag;
	float potential;
	float termWeight;
	
	// references
	unsigned int id1;
	unsigned int id2;

	// links
	int nFwArcs;
	int nBwArcs;
	struct ARC *fwArcsFirst;
	struct ARC *bwArcsFirst;
	struct ARC *fwArcsLast;
	struct ARC *bwArcsLast;
	
	// for ordering in a list
	struct NODE *nextLin;
	struct NODE *nextTmp;
};


struct ATT_BIN_HEADER
{
	unsigned int fsmClass;
	unsigned int semiring;
	unsigned int nNodes;
	unsigned int startNode;
};

struct ATT_BIN_NODE
{
	float potential;
	float termWeight;
	unsigned int nArcs;
};

struct ATT_BIN_ARC
{
	unsigned int labelFrom;
	unsigned int labelTo;
	float weight;
	unsigned int target;
};

struct NODE_INDEX
{
	struct NODE *node;
	int value;
};


class FSM
{
	protected:
		friend class FSM_ALGO;
		
		bool isTransducer;
		bool manageBWArcs;
		unsigned int startNodeId;
		unsigned int nNodes;
		unsigned int nArcs; 
		unsigned int semiring;
		unsigned int failureLabel;
		struct NODE *firstLin;
		struct NODE *lastLin;
		struct NODE *startNode;
		struct NODE_INDEX *nodeIndex;
		unsigned int maxNodeId;
		void GetSignature(FILE *fp, char *buff, int maxLen);
		void CreateNodeIndex();
		void DestroyNodeIndex();
		void NullIndexValues();
		void VerifySemiring();
		void AddBWArc(NODE *node, ARC *arc);
		// Counts number of incoming arcs for each node
		unsigned int CountIncomingArcs();

		FSMCache nodeCache;
		FSMCache arcCache;
	public:
		typedef int (*ARC_CMP_FUNC)(const void *a, const void *b);
		FSM();
		~FSM();
		void LoadBinAtt(char *file);
		void SaveBinAtt(char *file);
		void Erase();

		NODE *AddNode(unsigned int id = NODE_MAXID);
		NODE *GetNode(unsigned int id);
		NODE *GetStartNode() {return startNode;};
		void SetStartNode(NODE *node) {startNode = node; startNodeId = node->id;};
		NODE *GetFirstLinNode() {return firstLin;};
		NODE *GetLastLinNode() {return lastLin;};
		static NODE *GetNextNodeIS(NODE *node, unsigned int isymbol);
		static NODE *GetNextNodeOS(NODE *node, unsigned int osymbol);
		static void SetNodeTermWeight(NODE *node, float weight);
		static bool isTerminalNode(NODE *node) {return (node->flag & NF_TERMINAL);};
		static void SetNodeFlag(NODE *node, unsigned int flag) {node->flag |= flag;};
		static void ResetNodeFlag(NODE *node, unsigned int flag) {node->flag = node->flag & ~flag;};

		static ARC *GetNextArcIS(NODE *from, NODE *to, unsigned int isymbol);
		static ARC *GetNextArcOS(NODE *from, NODE *to, unsigned int osymbol);
		ARC *AddArc(NODE *node1, NODE *node2);
		void RemoveArcs(unsigned int minLabel, unsigned int maxLabel, unsigned int labelType);
		void RemoveBWArc(NODE *node, ARC *rmArc);
		void RemoveFreeNodes();
		static void CopyArcContent(ARC *from, ARC *to);

		void RenumberNodeIds();
		void SortArcs(ARC_CMP_FUNC fce = CmpArcToNodeId);
		void SortArcs(NODE *node, ARC_CMP_FUNC fce = CmpArcToNodeId);
		bool LoadFailureHeader(char *file, unsigned int *label);

		// arc comparison functions
		static int CmpArcToNodeId(const void *a, const void *b);
		static int CmpArcLabelFrom(const void *a, const void *b);
		static int CmpArcLabelTo(const void *a, const void *b);
		
		// informations
		bool GetIsTransducer() {VerifySemiring(); return isTransducer;};
		unsigned int GetStartNodeId() {return startNodeId;};
		unsigned int GetNodesNum() {return nNodes;};
		unsigned int GetArcsNum() {return nArcs;};
		unsigned int GetSemiring() {return semiring;};
		unsigned int GetFailureLabel() {return failureLabel;};
		
		static void Semiring2Str(unsigned int semiring, char *str);
		static void Bool2Str(bool v, char *str);

		// label shifting
		unsigned int GetMaxLabelLimited(unsigned int max, unsigned int labelType);
		void ShiftLabel(unsigned int minLabel, unsigned int maxLabel, long long shift, unsigned int labelType);
		void ReplaceLabels(unsigned int n, unsigned int *from, unsigned int *to, unsigned int labelType);
		void ReplaceLabel(unsigned int from, unsigned int to, unsigned int labelType) {ReplaceLabels(1, &from, &to, labelType);};

		// backward arcs management
		void SetManageBWArcs(bool flag) {manageBWArcs = flag; if(flag) CreateBWArcs();};
		void NullBWArcs();
		void CreateBWArcs();

		// automata modification
		void Convert2SVite();
};

class NODE_STACK
{
	protected:
		unsigned int nNodes;
		struct NODE *first;
		struct NODE *last;
	public:
		NODE_STACK();
		void Add(struct NODE *node);
		struct NODE *Get();
		bool IsEmpty() {return nNodes == 0;}
		unsigned int GetNodesNum() {return nNodes;};
};

struct COMPOSE_INDEX
{
	unsigned int id1;
	unsigned int id2;
	unsigned int id;
	struct NODE *node;
};

class FSM_ALGO
{
	public:
		FSM *compose(FSM *A, FSM *B);
};

#endif
