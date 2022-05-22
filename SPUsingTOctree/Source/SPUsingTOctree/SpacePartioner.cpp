// Fill out your copyright notice in the Description page of Project Settings.

#include "SpacePartioner.h"
#include "SPUsingTOctree.h"
//#include "DrawDebugHelpers.h"
#include "Engine/Public/DrawDebugHelpers.h"

// Sets default values
ASpacePartioner::ASpacePartioner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	PrintLogs = false;
	PrintTickLogs = false;

	bDrawDebugInfo = false;
	bInitialized = false;

	OctreeData = new FSimpleOctree(FVector(0.0f, 0.0f, 0.0f), 100.0f); // const FVector & InOrigin, float InExtent
}

void ASpacePartioner::Initialize(const FBox& inNewBounds, const bool& inDrawDebugInfo)
{
	bInitialized = true;
	bDrawDebugInfo = inDrawDebugInfo;
	OctreeData = new FSimpleOctree(inNewBounds.GetCenter(), inNewBounds.GetExtent().GetMax()); // const FVector & InOrigin, float InExtent
}

void ASpacePartioner::Initialize(const float& inExtent, const bool& inDrawDebugInfo)
{
	bInitialized = true;
	bDrawDebugInfo = inDrawDebugInfo;

	// The Extent is very similar to the radius of a circle
	FVector min = FVector(-inExtent, -inExtent, -inExtent);
	FVector max = FVector(inExtent, inExtent, inExtent);
	FBox NewBounds = FBox(min, max);
	OctreeData = new FSimpleOctree(NewBounds.GetCenter(), NewBounds.GetExtent().GetMax()); // const FVector & InOrigin, float InExtent
}

// Called when the game starts or when spawned
void ASpacePartioner::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASpacePartioner::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	if (bInitialized && bDrawDebugInfo)
	{
		int nodeCount = 0;
		int elementCount = 0;
		
		OctreeData->FindNodesWithPredicate(
			[this](FSimpleOctree::FNodeIndex /*ParentNodeIndex*/, FSimpleOctree::FNodeIndex /*NodeIndex*/, const FBoxCenterAndExtent& /*NodeBounds*/)
			{
				return true;
			},
			[this, &elementCount, &nodeCount](FSimpleOctree::FNodeIndex /*ParentNodeIndex*/, FSimpleOctree::FNodeIndex NodeIndex, const FBoxCenterAndExtent& NodeBounds)
			{
				nodeCount++;

				FVector maxExtent = NodeBounds.Extent;
				FVector center = NodeBounds.Center;

				DrawDebugBox(GetWorld(), center, maxExtent, FColor().Blue, false, 0.0f);
				DrawDebugSphere(GetWorld(), center + maxExtent, 4.0f, 12, FColor().Green, false, 0.0f);
				DrawDebugSphere(GetWorld(), center - maxExtent, 4.0f, 12, FColor().Red, false, 0.0f);

				TArrayView<const FOctreeElement> elements = OctreeData->GetElementsForNode(NodeIndex);

				for(int i = 0; i < elements.Num(); i++)
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
		
		if (PrintTickLogs) UE_LOG(LogTemp, Log, TEXT("Node Count: %d, Element Count: %d"), nodeCount, elementCount);
	}

	
}

void ASpacePartioner::AddOctreeElement(const FOctreeElement& inNewOctreeElement)
{
	check(bInitialized);
	OctreeData->AddElement(inNewOctreeElement);
	if (PrintLogs) UE_LOG(LogTemp, Log, TEXT("Added element to Octree."));
}

TArray<FOctreeElement> ASpacePartioner::GetElementsWithinBounds(const FBoxSphereBounds& inBoundingBoxQuery, const bool bSphereOnlyTest = false, const bool bDrawDebug = false, const bool bPersistentLines = false, const float lifeTime = 0.0f)
{
	if (bDrawDebug)
	{
		DrawBoxSphereBounds(inBoundingBoxQuery, bSphereOnlyTest, bPersistentLines, lifeTime);
	}
	TArray<FOctreeElement> octreeElements;
	FBox box = inBoundingBoxQuery.GetBox();
	FSphere sphere = inBoundingBoxQuery.GetSphere();
	
	//--just for reference--
	//OctreeData->FindAllElements([&octreeElements, &box, &sphere](const FOctreeElement& octElement)
	//	{
	//		if (box.IsInside(octElement.BoxSphereBounds.GetBox()) || sphere.IsInside(octElement.MyActor->GetActorLocation()))
	//		{
	//			octreeElements.Add(octElement);
	//		}
	//	});
	
	OctreeData->FindNodesWithPredicate(
		[&box](FSimpleOctree::FNodeIndex /*ParentNodeIndex*/, FSimpleOctree::FNodeIndex /*NodeIndex*/, const FBoxCenterAndExtent& NodeBounds)
		{
			if (NodeBounds.GetBox().IsInside(box.GetCenter()) || NodeBounds.GetBox().Intersect(box))
			{
				return true;
			}
			return false;
		},
		[this, &octreeElements, &box, &sphere, &bSphereOnlyTest](FSimpleOctree::FNodeIndex /*ParentNodeIndex*/, FSimpleOctree::FNodeIndex NodeIndex, const FBoxCenterAndExtent& /*NodeBounds*/)
		{
			TArrayView<const FOctreeElement> elements = OctreeData->GetElementsForNode(NodeIndex);
			
			if (PrintLogs) UE_LOG(LogTemp, Log, TEXT("GetElementsWithinBounds NodeIndex: %d"), NodeIndex);

			for (int i = 0; i < elements.Num(); i++)
			{
				if (
					(!bSphereOnlyTest &&(box.IsInside(elements[i].BoxSphereBounds.GetBox()) || sphere.IsInside(elements[i].MyActor->GetActorLocation()))) 
					||
					(bSphereOnlyTest && sphere.IsInside(elements[i].MyActor->GetActorLocation()))
					)
				{
					octreeElements.Add(elements[i]);
				}
			}
		});

	if (PrintLogs) UE_LOG(LogTemp, Log, TEXT("GetElementsWithinBounds octreeElements: %d"), octreeElements.Num());

	return octreeElements;
}

void ASpacePartioner::DrawBoxSphereBounds(const FBoxSphereBounds& inBoundingBoxQuery, const bool bSphereOnlyTest = false, const bool bPersistentLines = false, const float lifeTime = 0.0f)
{
	DrawDebugBox(GetWorld(), inBoundingBoxQuery.Origin, inBoundingBoxQuery.BoxExtent, (bSphereOnlyTest ? FColor(128.0f, 128.0f, 128.0f, 128.0f) : FColor().Purple), bPersistentLines, lifeTime);
	DrawDebugSphere(GetWorld(), inBoundingBoxQuery.Origin, inBoundingBoxQuery.SphereRadius, 12, (bSphereOnlyTest ? FColor().Orange : FColor().Turquoise), bPersistentLines, lifeTime, 0, (bSphereOnlyTest ? 3.0f : 0.0f));
}

void ASpacePartioner::DrawOctreeBounds()
{
	FVector extent = this->OctreeData->GetRootBounds().Extent;
	
	float max = extent.GetMax();
	FVector maxExtent = FVector(max, max, max);
	FVector center = this->OctreeData->GetRootBounds().Center;

	DrawDebugBox(GetWorld(), center, maxExtent, FColor().Green, false, 0.0f);
	DrawDebugSphere(GetWorld(), center + maxExtent, 4.0f, 12, FColor().White, false, 0.0f);
	DrawDebugSphere(GetWorld(), center - maxExtent, 4.0f, 12, FColor().White, false, 0.0f);
}