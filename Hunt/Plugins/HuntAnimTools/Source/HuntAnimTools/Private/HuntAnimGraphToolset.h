#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "HuntAnimGraphToolset.generated.h"

class UBlueprint;
class UEdGraph;
class UEdGraphNode;

/// Edits Animation Blueprint graphs the way a person does in the editor. Use it for what BlueprintTools cannot do:
/// state machine states and transitions, deleting anim nodes, copying or moving nodes between graphs, adding blend pins.
/// Regular nodes, pin links and pin values still go through BlueprintTools; node properties through ObjectTools.
UCLASS(BlueprintType)
class UHuntAnimGraphToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * Lists every graph in a Blueprint, including state, transition rule and nested state machine graphs.
	 * @param Blueprint The Blueprint to inspect.
	 * @return Object paths of all graphs.
	 */
	UFUNCTION(meta = (AICallable), Category = "HuntAnimGraph")
	static TArray<FString> ListGraphs(UBlueprint* Blueprint);

	/**
	 * Describes every node of a graph as JSON. Unlike get_node_infos it also works on state, conduit and transition nodes.
	 * @param Graph The graph to describe.
	 * @return JSON array of nodes: path, class, title, x, y, subgraphs and pins (name, direction, default, links).
	 */
	UFUNCTION(meta = (AICallable), Category = "HuntAnimGraph")
	static FString DescribeGraph(UEdGraph* Graph);

	/**
	 * Adds a state to a state machine graph.
	 * @param StateMachineGraph The state machine graph.
	 * @param Name The state name.
	 * @param X Graph X position.
	 * @param Y Graph Y position.
	 * @return The new state node. Its subgraphs entry in DescribeGraph is the graph to fill.
	 */
	UFUNCTION(meta = (AICallable), Category = "HuntAnimGraph")
	static UEdGraphNode* AddState(UEdGraph* StateMachineGraph, const FString& Name, int32 X = 0, int32 Y = 0);

	/**
	 * Adds a conduit to a state machine graph.
	 * @param StateMachineGraph The state machine graph.
	 * @param Name The conduit name.
	 * @param X Graph X position.
	 * @param Y Graph Y position.
	 * @return The new conduit node.
	 */
	UFUNCTION(meta = (AICallable), Category = "HuntAnimGraph")
	static UEdGraphNode* AddConduit(UEdGraph* StateMachineGraph, const FString& Name, int32 X = 0, int32 Y = 0);

	/**
	 * Adds a transition arrow between two states or conduits.
	 * @param From The source state or conduit.
	 * @param To The destination state or conduit.
	 * @return The new transition node. Set CrossfadeDuration and similar with ObjectTools; its rule graph is in subgraphs.
	 */
	UFUNCTION(meta = (AICallable), Category = "HuntAnimGraph")
	static UEdGraphNode* AddTransition(UEdGraphNode* From, UEdGraphNode* To);

	/**
	 * Connects the state machine's entry point to a state.
	 * @param StateMachineGraph The state machine graph.
	 * @param State The state to enter first.
	 * @return True if connected.
	 */
	UFUNCTION(meta = (AICallable), Category = "HuntAnimGraph")
	static bool SetEntryState(UEdGraph* StateMachineGraph, UEdGraphNode* State);

	/**
	 * Renames a state, conduit or other renamable node.
	 * @param Node The node to rename.
	 * @param Name The new name.
	 * @return True if the node can be renamed.
	 */
	UFUNCTION(meta = (AICallable), Category = "HuntAnimGraph")
	static bool RenameNode(UEdGraphNode* Node, const FString& Name);

	/**
	 * Deletes nodes of any kind, as the Delete key does. Transitions attached to a deleted state are deleted with it.
	 * @param Nodes The nodes to delete.
	 * @return How many nodes were deleted.
	 */
	UFUNCTION(meta = (AICallable), Category = "HuntAnimGraph")
	static int32 RemoveNodes(const TArray<UEdGraphNode*>& Nodes);

	/**
	 * Copies nodes into a graph, as Ctrl+C and Ctrl+V do. Links between the copied nodes are kept, links to other nodes are not.
	 * @param Nodes The nodes to copy.
	 * @param TargetGraph The graph to paste into.
	 * @param X Graph X position of the pasted group's center.
	 * @param Y Graph Y position of the pasted group's center.
	 * @return The pasted nodes.
	 */
	UFUNCTION(meta = (AICallable), Category = "HuntAnimGraph")
	static TArray<UEdGraphNode*> CopyNodes(const TArray<UEdGraphNode*>& Nodes, UEdGraph* TargetGraph, int32 X = 0, int32 Y = 0);

	/**
	 * Moves nodes into another graph, as Ctrl+X and Ctrl+V do. Reconnect outside links afterwards.
	 * @param Nodes The nodes to move.
	 * @param TargetGraph The graph to paste into.
	 * @param X Graph X position of the pasted group's center.
	 * @param Y Graph Y position of the pasted group's center.
	 * @return The nodes at their new place.
	 */
	UFUNCTION(meta = (AICallable), Category = "HuntAnimGraph")
	static TArray<UEdGraphNode*> MoveNodes(const TArray<UEdGraphNode*>& Nodes, UEdGraph* TargetGraph, int32 X = 0, int32 Y = 0);

	/**
	 * Adds a blend pin to a Layered Blend per Bone or Blend Poses by Int node.
	 * @param Node The blend node.
	 * @return True if a pin was added.
	 */
	UFUNCTION(meta = (AICallable), Category = "HuntAnimGraph")
	static bool AddBlendPin(UEdGraphNode* Node);

	/**
	 * Compiles a Blueprint and returns the compiler messages, which compile_blueprint does not return.
	 * @param Blueprint The Blueprint to compile.
	 * @return One line per message with its severity. Empty when clean.
	 */
	UFUNCTION(meta = (AICallable), Category = "HuntAnimGraph")
	static TArray<FString> CompileWithMessages(UBlueprint* Blueprint);
};
