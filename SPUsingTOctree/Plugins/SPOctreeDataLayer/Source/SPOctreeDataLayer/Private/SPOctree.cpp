// Fill out your copyright notice in the Description page of Project Settings.

#include "SPOctree.h"
#include "SPOctreeDataLayer.h"
#include "Engine/Public/DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ASPOctree::ASPOctree(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	PrintLogs = false;
	PrintTickLogs = false;

	bDrawDebugInfo = false;
	bInitialized = false;

	OctreeData = new FSPOctree(FVector(0.0f, 0.0f, 0.0f), 100.0f); // const FVector & InOrigin, float InExtent
}

void ASPOctree::Initialize(const FBox& inNewBounds, const bool& inDrawDebugInfo)
{
	bInitialized = true;
	bDrawDebugInfo = inDrawDebugInfo;
	OctreeData = new FSPOctree(inNewBounds.GetCenter(), inNewBounds.GetExtent().GetMax()); // const FVector & InOrigin, float InExtent
}

void ASPOctree::Initialize(const float& inExtent, const bool& inDrawDebugInfo)
{
	bInitialized = true;
	bDrawDebugInfo = inDrawDebugInfo;

	// The Extent is very similar to the radius of a circle
	FVector min = FVector(-inExtent, -inExtent, -inExtent);
	FVector max = FVector(inExtent, inExtent, inExtent);
	FBox NewBounds = FBox(min, max);
	OctreeData = new FSPOctree(NewBounds.GetCenter(), NewBounds.GetExtent().GetMax()); // const FVector & InOrigin, float InExtent
}

// Called when the game starts or when spawned
void ASPOctree::BeginPlay()
{
	Super::BeginPlay();
}

void ASPOctree::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OctreeData->Destroy();
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void ASPOctree::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bInitialized && bDrawDebugInfo)
	{
		int nodeCount = 0;
		int elementCount = 0;

		OctreeData->FindNodesWithPredicate(
			[this](FSPOctree::FNodeIndex /*ParentNodeIndex*/, FSPOctree::FNodeIndex /*NodeIndex*/, const FBoxCenterAndExtent& /*NodeBounds*/)
			{
				return true;
			},
			[this, &elementCount, &nodeCount](FSPOctree::FNodeIndex /*ParentNodeIndex*/, FSPOctree::FNodeIndex NodeIndex, const FBoxCenterAndExtent& NodeBounds)
			{
				nodeCount++;

				FVector maxExtent = NodeBounds.Extent;
				FVector center = NodeBounds.Center;

				DrawDebugBox(GetWorld(), center, maxExtent, FColor().Blue, false, 0.0f);
				DrawDebugSphere(GetWorld(), center + maxExtent, 4.0f, 12, FColor().Green, false, 0.0f);
				DrawDebugSphere(GetWorld(), center - maxExtent, 4.0f, 12, FColor().Red, false, 0.0f);

				TArrayView<const FSPOctreeElement> elements = OctreeData->GetElementsForNode(NodeIndex);

				for (int i = 0; i < elements.Num(); i++)
				{
					// Draw debug boxes around elements
					float max = elements[i].BoxSphereBounds.BoxExtent.GetMax();
					maxExtent = FVector(max, max, max);
					center = elements[i].MyActor->GetActorLocation();

					DrawDebugBox(GetWorld(), center, maxExtent, FColor().Yellow, false, 0.0f);
					DrawDebugSphere(GetWorld(), center + maxExtent, 4.0f, 12, FColor().White, false, 0.0f);
					DrawDebugSphere(GetWorld(), center - maxExtent, 4.0f, 12, FColor().White, false, 0.0f);
					elementCount++;
				}
			});

		if (PrintTickLogs) UE_LOG(SPOctreeDataLayerMod, Log, TEXT("Node Count: %d, Element Count: %d"), nodeCount, elementCount);
	}


}

void ASPOctree::AddOctreeElement(const FSPOctreeElement& inNewOctreeElement, const bool inHiddenInGame)
{
	check(bInitialized);
	OctreeData->AddElement(inNewOctreeElement);
	inNewOctreeElement.MyActor->SetActorHiddenInGame(inHiddenInGame);
	inNewOctreeElement.MyActor->SetActorEnableCollision(!inHiddenInGame);
	if (PrintLogs) UE_LOG(SPOctreeDataLayerMod, Log, TEXT("Added element [%s] to Octree."), *(inNewOctreeElement.MyActor->GetName()));
}

TArray<FSPOctreeElement> ASPOctree::GetElementsWithinBounds(const FBoxSphereBounds& inBoundingBoxQuery, const bool bSphereOnlyTest = false, const bool bDrawDebug = false, const bool bPersistentLines = false, const float lifeTime = 0.0f)
{
	if (bDrawDebug)
	{
		DrawBoxSphereBounds(inBoundingBoxQuery, bSphereOnlyTest, bPersistentLines, lifeTime);
	}
	TArray<FSPOctreeElement> octreeElements;
	octreeElements.Reset();
	FBox box = inBoundingBoxQuery.GetBox();
	FSphere sphere = inBoundingBoxQuery.GetSphere();
	FBox sphereBox = FBox(FVector(sphere.Center.X - sphere.W, sphere.Center.Y - sphere.W, sphere.Center.Z - sphere.W),
		FVector(sphere.Center.X + sphere.W, sphere.Center.Y + sphere.W, sphere.Center.Z + sphere.W));

	OctreeData->FindNodesWithPredicate(
		[&box, &sphereBox](FSPOctree::FNodeIndex /*ParentNodeIndex*/, FSPOctree::FNodeIndex /*NodeIndex*/, const FBoxCenterAndExtent& NodeBounds)
		{
			if (NodeBounds.GetBox().IsInside(box.GetCenter()) || NodeBounds.GetBox().Intersect(box) || NodeBounds.GetBox().Intersect(sphereBox))
			{
				return true;
			}
			return false;
		},
		[this, &octreeElements, &box, &sphere, &bSphereOnlyTest](FSPOctree::FNodeIndex /*ParentNodeIndex*/, FSPOctree::FNodeIndex NodeIndex, const FBoxCenterAndExtent& /*NodeBounds*/)
		{
			TArrayView<const FSPOctreeElement> elements = OctreeData->GetElementsForNode(NodeIndex);
			int numElements = elements.Num();
			if (PrintLogs) UE_LOG(SPOctreeDataLayerMod, Log, TEXT("GetElementsWithinBounds NodeIndex: %d numElements: %i"), NodeIndex, numElements);

			for (int Index = 0; Index < numElements; Index++)
			{
				if (
					(!bSphereOnlyTest && (box.IsInside(elements[Index].BoxSphereBounds.GetBox()) || box.Intersect(elements[Index].BoxSphereBounds.GetBox()) || sphere.IsInside(elements[Index].MyActor->GetActorLocation())))
					||
					(bSphereOnlyTest && sphere.IsInside(elements[Index].MyActor->GetActorLocation()))
					)
				{
					octreeElements.Add(elements[Index]);
					if (PrintLogs) UE_LOG(SPOctreeDataLayerMod, Log, TEXT("GetElementsWithinBounds elements[%i].MyActor: %s"), Index, *(elements[Index].MyActor->GetActorNameOrLabel()));
				}
			}
		});

	if (PrintLogs) UE_LOG(SPOctreeDataLayerMod, Log, TEXT("GetElementsWithinBounds octreeElements: %d"), octreeElements.Num());

	return octreeElements;
}

void ASPOctree::DrawBoxSphereBounds(const FBoxSphereBounds& inBoundingBoxQuery, const bool bSphereOnlyTest = false, const bool bPersistentLines = false, const float lifeTime = 0.0f)
{
	DrawDebugBox(GetWorld(), inBoundingBoxQuery.Origin, inBoundingBoxQuery.BoxExtent, (bSphereOnlyTest ? FColor(128.0f, 128.0f, 128.0f, 128.0f) : FColor().Purple), bPersistentLines, lifeTime);
	DrawDebugSphere(GetWorld(), inBoundingBoxQuery.Origin, inBoundingBoxQuery.SphereRadius, 12, (bSphereOnlyTest ? FColor().Orange : FColor().Turquoise), bPersistentLines, lifeTime, 0, (bSphereOnlyTest ? 3.0f : 0.0f));
}

void ASPOctree::GetAllActorsWithinBounds(const FBoxSphereBounds& inBoundingBoxQuery, TSubclassOf<AActor> ActorClass, TArray<AActor*>& OutActors, const bool bSphereOnlyTest, const bool bDrawDebug, const bool bPersistentLines, const float lifeTime)
{
	if (bDrawDebug)
	{
		DrawDebugBox(GetWorld(), inBoundingBoxQuery.Origin, inBoundingBoxQuery.BoxExtent, (bSphereOnlyTest ? FColor(128.0f, 128.0f, 128.0f, 128.0f) : FColor().Red), bPersistentLines, lifeTime);
		DrawDebugSphere(GetWorld(), inBoundingBoxQuery.Origin, inBoundingBoxQuery.SphereRadius, 12, (bSphereOnlyTest ? FColor().Cyan : FColor().Red), bPersistentLines, lifeTime, 0, (bSphereOnlyTest ? 3.0f : 0.0f));
	}
	OutActors.Reset();

	FBox box = inBoundingBoxQuery.GetBox();
	FSphere sphere = inBoundingBoxQuery.GetSphere();
	TArray<AActor*> foundActors;
	foundActors.Reset();
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ActorClass, foundActors);

	int numActors = foundActors.Num();
	if (PrintLogs) UE_LOG(SPOctreeDataLayerMod, Log, TEXT("GetAllActorsWithinBounds numActors: %d"), numActors);

	for (int Index = 0; Index < numActors; Index++)
	{
		FBox actorBounds = foundActors[Index]->GetComponentsBoundingBox(true, true);
		if (
			(!bSphereOnlyTest && (box.IsInside(actorBounds) || box.Intersect(actorBounds) || sphere.IsInside(foundActors[Index]->GetActorLocation())))
			||
			(bSphereOnlyTest && sphere.IsInside(foundActors[Index]->GetActorLocation()))
			)
		{
			OutActors.Add(foundActors[Index]);
		}
		else
		{
			if (PrintLogs) UE_LOG(SPOctreeDataLayerMod, Log, TEXT("GetAllActorsWithinBounds: foundActors[%i]: %s NOT in bounds."), Index, *(foundActors[Index]->GetActorNameOrLabel()));
		}
	}

	if (PrintLogs) UE_LOG(SPOctreeDataLayerMod, Log, TEXT("GetAllActorsWithinBounds OutActors: %d"), OutActors.Num());
}

void ASPOctree::AddActorToOctree(AActor* inActor, const bool inHiddenInGame)
{
	if (inActor)
	{
		FVector origin;
		FVector boxExtent;
		inActor->GetActorBounds(false, origin, boxExtent);
		double maxExtent = boxExtent.GetMax();

		if (maxExtent < OctreeData->GetRootBounds().GetBox().GetExtent().GetMax())
		{
			check(bInitialized);
			FSPOctreeElement element = FSPOctreeElement(inActor, FBoxSphereBounds(origin, boxExtent, maxExtent));
			inActor->SetActorHiddenInGame(inHiddenInGame);
			inActor->SetActorEnableCollision(!inHiddenInGame);
			OctreeData->AddElement(element);
			if (PrintLogs) UE_LOG(SPOctreeDataLayerMod, Log, TEXT("AddActorToOctree: [%s] to Octree."), *(inActor->GetActorNameOrLabel()));
		}
		else
		{
			//for things like skysphere
			if (PrintLogs) UE_LOG(SPOctreeDataLayerMod, Log, TEXT("AddActorToOctree: maxExtent greater than OctreeData->GetRootBounds maxExtent!"));
		}
	}
}

void ASPOctree::GetAllActors(TArray<AActor*>& OutActors)
{
	OutActors.Reset();

	OctreeData->FindAllElements([&OutActors](const FSPOctreeElement& octElement)
		{
			if (octElement.MyActor)
			{
				OutActors.Add(octElement.MyActor);
			}
		});
}

void ASPOctree::DrawOctreeBounds()
{
	FVector extent = this->OctreeData->GetRootBounds().Extent;

	float max = extent.GetMax();
	FVector maxExtent = FVector(max, max, max);
	FVector center = this->OctreeData->GetRootBounds().Center;

	DrawDebugBox(GetWorld(), center, maxExtent, FColor().Green, false, 0.0f);
	DrawDebugSphere(GetWorld(), center + maxExtent, 4.0f, 12, FColor().White, false, 0.0f);
	DrawDebugSphere(GetWorld(), center - maxExtent, 4.0f, 12, FColor().White, false, 0.0f);
}
