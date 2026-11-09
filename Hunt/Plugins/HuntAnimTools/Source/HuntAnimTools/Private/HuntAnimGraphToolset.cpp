#include "HuntAnimGraphToolset.h"

#include "AnimGraphNode_BlendListByInt.h"
#include "AnimGraphNode_LayeredBoneBlend.h"
#include "AnimStateConduitNode.h"
#include "AnimStateEntryNode.h"
#include "AnimStateNode.h"
#include "AnimStateTransitionNode.h"
#include "AnimationStateMachineSchema.h"
#include "Dom/JsonObject.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "ToolsetRegistry/UToolsetRegistry.h"

namespace
{
	void MarkStructural(const UEdGraph* Graph)
	{
		if (UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph))
		{
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		}
	}

	UEdGraphPin* FindDirectionPin(const UEdGraphNode* Node, EEdGraphPinDirection Direction)
	{
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin->Direction == Direction)
			{
				return Pin;
			}
		}
		return nullptr;
	}

	template <typename NodeType>
	UEdGraphNode* AddStateMachineNode(UEdGraph* Graph, const FString& Name, int32 X, int32 Y)
	{
		if (!Graph || !Graph->GetSchema()->IsA<UAnimationStateMachineSchema>())
		{
			return nullptr;
		}
		UEdGraphNode* Node = FEdGraphSchemaAction_NewStateNode::SpawnNodeFromTemplate<NodeType>(
			Graph, NewObject<NodeType>(), FVector2f(X, Y), false);
		if (Node && !Name.IsEmpty())
		{
			Node->OnRenameNode(Name);
		}
		MarkStructural(Graph);
		return Node;
	}
}

TArray<FString> UHuntAnimGraphToolset::ListGraphs(UBlueprint* Blueprint)
{
	TArray<FString> Paths;
	if (Blueprint)
	{
		TArray<UEdGraph*> Graphs;
		Blueprint->GetAllGraphs(Graphs);
		for (const UEdGraph* Graph : Graphs)
		{
			Paths.Add(Graph->GetPathName());
		}
	}
	return Paths;
}

FString UHuntAnimGraphToolset::DescribeGraph(UEdGraph* Graph)
{
	TArray<TSharedPtr<FJsonValue>> JsonNodes;
	if (Graph)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			const TSharedRef<FJsonObject> JsonNode = MakeShared<FJsonObject>();
			JsonNode->SetStringField(TEXT("path"), Node->GetPathName());
			JsonNode->SetStringField(TEXT("class"), Node->GetClass()->GetName());
			JsonNode->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
			JsonNode->SetNumberField(TEXT("x"), Node->NodePosX);
			JsonNode->SetNumberField(TEXT("y"), Node->NodePosY);

			TArray<TSharedPtr<FJsonValue>> SubGraphs;
			for (const UEdGraph* SubGraph : Node->GetSubGraphs())
			{
				if (SubGraph) // 전환 노드의 커스텀 블렌드 그래프는 보통 비어 있다
				{
					SubGraphs.Add(MakeShared<FJsonValueString>(SubGraph->GetPathName()));
				}
			}
			JsonNode->SetArrayField(TEXT("subgraphs"), SubGraphs);

			TArray<TSharedPtr<FJsonValue>> JsonPins;
			for (const UEdGraphPin* Pin : Node->Pins)
			{
				const TSharedRef<FJsonObject> JsonPin = MakeShared<FJsonObject>();
				JsonPin->SetStringField(TEXT("name"), Pin->PinName.ToString());
				JsonPin->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
				JsonPin->SetStringField(TEXT("default"), Pin->GetDefaultAsString());
				TArray<TSharedPtr<FJsonValue>> Links;
				for (const UEdGraphPin* Linked : Pin->LinkedTo)
				{
					Links.Add(MakeShared<FJsonValueString>(Linked->GetOwningNode()->GetName() + TEXT(".") + Linked->PinName.ToString()));
				}
				JsonPin->SetArrayField(TEXT("links"), Links);
				JsonPins.Add(MakeShared<FJsonValueObject>(JsonPin));
			}
			JsonNode->SetArrayField(TEXT("pins"), JsonPins);
			JsonNodes.Add(MakeShared<FJsonValueObject>(JsonNode));
		}
	}
	FString Output;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(JsonNodes, Writer);
	return Output;
}

UEdGraphNode* UHuntAnimGraphToolset::AddState(UEdGraph* StateMachineGraph, const FString& Name, int32 X, int32 Y)
{
	return AddStateMachineNode<UAnimStateNode>(StateMachineGraph, Name, X, Y);
}

UEdGraphNode* UHuntAnimGraphToolset::AddConduit(UEdGraph* StateMachineGraph, const FString& Name, int32 X, int32 Y)
{
	return AddStateMachineNode<UAnimStateConduitNode>(StateMachineGraph, Name, X, Y);
}

UEdGraphNode* UHuntAnimGraphToolset::AddTransition(UEdGraphNode* From, UEdGraphNode* To)
{
	if (!From || !To || From->GetGraph() != To->GetGraph())
	{
		return nullptr;
	}
	UEdGraphPin* Output = FindDirectionPin(From, EGPD_Output);
	UEdGraphPin* Input = FindDirectionPin(To, EGPD_Input);
	if (!Output || !Input)
	{
		return nullptr;
	}
	const TArray<UEdGraphPin*> Before = Output->LinkedTo;
	// 스키마가 사람이 화살표를 그릴 때와 같은 경로로 전환 노드를 만든다
	if (!From->GetGraph()->GetSchema()->TryCreateConnection(Output, Input))
	{
		return nullptr;
	}
	MarkStructural(From->GetGraph());
	for (UEdGraphPin* Linked : Output->LinkedTo)
	{
		if (!Before.Contains(Linked))
		{
			return Linked->GetOwningNode();
		}
	}
	return nullptr;
}

bool UHuntAnimGraphToolset::SetEntryState(UEdGraph* StateMachineGraph, UEdGraphNode* State)
{
	if (!StateMachineGraph || !State)
	{
		return false;
	}
	TArray<UAnimStateEntryNode*> Entries;
	StateMachineGraph->GetNodesOfClass(Entries);
	UEdGraphPin* Output = Entries.Num() > 0 ? FindDirectionPin(Entries[0], EGPD_Output) : nullptr;
	UEdGraphPin* Input = FindDirectionPin(State, EGPD_Input);
	if (!Output || !Input)
	{
		return false;
	}
	Output->BreakAllPinLinks();
	const bool bConnected = StateMachineGraph->GetSchema()->TryCreateConnection(Output, Input);
	MarkStructural(StateMachineGraph);
	return bConnected;
}

bool UHuntAnimGraphToolset::RenameNode(UEdGraphNode* Node, const FString& Name)
{
	if (!Node || !Node->GetCanRenameNode())
	{
		return false;
	}
	Node->OnRenameNode(Name);
	MarkStructural(Node->GetGraph());
	return true;
}

int32 UHuntAnimGraphToolset::RemoveNodes(const TArray<UEdGraphNode*>& Nodes)
{
	// 상태를 지우면 붙어 있던 전환도 같이 지운다. 전환은 양쪽 상태가 있어야 유효하다
	TSet<UEdGraphNode*> ToRemove;
	for (UEdGraphNode* Node : Nodes)
	{
		if (!Node)
		{
			continue;
		}
		ToRemove.Add(Node);
		if (Node->GetSchema()->IsA<UAnimationStateMachineSchema>())
		{
			for (const UEdGraphPin* Pin : Node->Pins)
			{
				for (const UEdGraphPin* Linked : Pin->LinkedTo)
				{
					if (UAnimStateTransitionNode* Transition = Cast<UAnimStateTransitionNode>(Linked->GetOwningNode()))
					{
						ToRemove.Add(Transition);
					}
				}
			}
		}
	}
	int32 Removed = 0;
	for (UEdGraphNode* Node : ToRemove)
	{
		if (Node->CanUserDeleteNode())
		{
			UEdGraph* Graph = Node->GetGraph();
			Graph->Modify();
			FBlueprintEditorUtils::RemoveNode(FBlueprintEditorUtils::FindBlueprintForGraph(Graph), Node, true);
			++Removed;
		}
	}
	return Removed;
}

TArray<UEdGraphNode*> UHuntAnimGraphToolset::CopyNodes(const TArray<UEdGraphNode*>& Nodes, UEdGraph* TargetGraph, int32 X, int32 Y)
{
	TArray<UEdGraphNode*> Result;
	if (!TargetGraph)
	{
		return Result;
	}
	FGraphPanelSelectionSet Selection;
	for (UEdGraphNode* Node : Nodes)
	{
		if (Node && Node->CanDuplicateNode())
		{
			Node->PrepareForCopying();
			Selection.Add(Node);
		}
	}
	FString Text;
	FEdGraphUtilities::ExportNodesToText(Selection, Text);
	if (!FEdGraphUtilities::CanImportNodesFromText(TargetGraph, Text))
	{
		return Result;
	}

	TargetGraph->Modify();
	TSet<UEdGraphNode*> Pasted;
	FEdGraphUtilities::ImportNodesFromText(TargetGraph, Text, Pasted);
	if (Pasted.Num() == 0)
	{
		return Result;
	}
	FVector2D Center = FVector2D::ZeroVector;
	for (const UEdGraphNode* Node : Pasted)
	{
		Center += FVector2D(Node->NodePosX, Node->NodePosY);
	}
	Center /= Pasted.Num();
	for (UEdGraphNode* Node : Pasted)
	{
		Node->NodePosX = int32(Node->NodePosX - Center.X) + X;
		Node->NodePosY = int32(Node->NodePosY - Center.Y) + Y;
		Node->CreateNewGuid();
		Result.Add(Node);
	}
	MarkStructural(TargetGraph);
	TargetGraph->NotifyGraphChanged();
	return Result;
}

TArray<UEdGraphNode*> UHuntAnimGraphToolset::MoveNodes(const TArray<UEdGraphNode*>& Nodes, UEdGraph* TargetGraph, int32 X, int32 Y)
{
	TArray<UEdGraphNode*> Result = CopyNodes(Nodes, TargetGraph, X, Y);
	if (Result.Num() > 0)
	{
		RemoveNodes(Nodes);
	}
	return Result;
}

bool UHuntAnimGraphToolset::AddBlendPin(UEdGraphNode* Node)
{
	if (UAnimGraphNode_LayeredBoneBlend* Layered = Cast<UAnimGraphNode_LayeredBoneBlend>(Node))
	{
		Layered->AddPinToBlendByFilter();
	}
	else if (UAnimGraphNode_BlendListByInt* BlendList = Cast<UAnimGraphNode_BlendListByInt>(Node))
	{
		BlendList->AddPinToBlendList();
	}
	else
	{
		return false;
	}
	MarkStructural(Node->GetGraph());
	return true;
}

TArray<FString> UHuntAnimGraphToolset::CompileWithMessages(UBlueprint* Blueprint)
{
	TArray<FString> Messages;
	if (Blueprint)
	{
		FCompilerResultsLog Log;
		Log.SetSourcePath(Blueprint->GetPathName());
		FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipGarbageCollection, &Log);
		for (const TSharedRef<FTokenizedMessage>& Message : Log.Messages)
		{
			Messages.Add(Message->ToText().ToString());
		}
	}
	return Messages;
}

class FHuntAnimToolsModule : public IModuleInterface
{
	virtual void StartupModule() override
	{
		UToolsetRegistry::RegisterToolsetClass(UHuntAnimGraphToolset::StaticClass());
	}

	virtual void ShutdownModule() override
	{
		UToolsetRegistry::UnregisterToolsetClass(UHuntAnimGraphToolset::StaticClass());
	}
};

IMPLEMENT_MODULE(FHuntAnimToolsModule, HuntAnimTools)
