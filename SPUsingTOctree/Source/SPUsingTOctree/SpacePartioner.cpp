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

	//check(bInitialized);
	//check(bDrawDebugInfo);

	if (bInitialized && bDrawDebugInfo)
	{
		int level = 0;
		float max = 0;
		float offsetMax = 0;
		float offset = 0;
		FVector maxExtent = FVector(0, 0, 0);
		FVector center = FVector(0, 0, 0);
		FVector tempForCoercion = FVector(0, 0, 0);;
		FBoxCenterAndExtent OldBounds = FBoxCenterAndExtent();

		int nodeCount = 0;
		int elementCount = 0;
		
		OctreeData->FindNodesWithPredicate(
			[this](FSimpleOctree::FNodeIndex /*ParentNodeIndex*/, FSimpleOctree::FNodeIndex /*NodeIndex*/, const FBoxCenterAndExtent& /*NodeBounds*/)
			{
				return true;
			},
			[this, &tempForCoercion, &max, &center, &OldBounds, &level, &offsetMax, &offset, &maxExtent, &elementCount, &nodeCount](FSimpleOctree::FNodeIndex /*ParentNodeIndex*/, FSimpleOctree::FNodeIndex NodeIndex, const FBoxCenterAndExtent& NodeBounds)
			{
				nodeCount++;

				// If the extents have changed then we have moved a level.
				if (!OldBounds.Extent.Equals(NodeBounds.Extent))
				{
					level++;
				}
				OldBounds = NodeBounds;
				
				//UE_LOG(LogTemp, Log, TEXT("Level: %d"), level);
				
				// Draw Node Bounds
				tempForCoercion = NodeBounds.Extent;
				max = tempForCoercion.GetMax();
				center = NodeBounds.Center;

				//UE_LOG(LogTemp, Log, TEXT("center before: %s"), *center.ToString());

				// To understand the math here check out the constructors in FOctreeNodeContext
				// Offset nodes that are not the root bounds
				if (!OctreeData->GetRootBounds().Extent.Equals(NodeBounds.Extent))
				{
					for (int i = 1; i < level; i++)
					{
						// Calculate offset
						offsetMax = max / (1.0f + (1.0f / FOctreeNodeContext::LoosenessDenominator));
						offset = max - offsetMax;
						max = offsetMax;

						// Calculate Center Offset
						if (center.X > 0)
						{
							center.X = center.X + offset;
						}
						else
						{
							center.X = center.X - offset;
						}

						if (center.Y > 0)
						{
							center.Y = center.Y + offset;
						}
						else
						{
							center.Y = center.Y - offset;
						}

						if (center.Z > 0)
						{
							center.Z = center.Z + offset;
						}
						else
						{
							center.Z = center.Z - offset;
						}
					}
				}

				//UE_LOG(LogTemp, Log, TEXT("max: %f"), max);
				//UE_LOG(LogTemp, Log, TEXT("center of nodes: %s"), *center.ToString());

				maxExtent = FVector(max, max, max);

				//UE_LOG(LogTemp, Log, TEXT("Extent of nodes: %s"), *tempForCoercion.ToString());

				DrawDebugBox(GetWorld(), center, maxExtent, FColor().Blue, false, 0.0f);
				DrawDebugSphere(GetWorld(), center + maxExtent, 4.0f, 12, FColor().Green, false, 0.0f);
				DrawDebugSphere(GetWorld(), center - maxExtent, 4.0f, 12, FColor().Red, false, 0.0f);

				TArrayView<const FOctreeElement> elements = OctreeData->GetElementsForNode(NodeIndex);

				for(int i = 0; i < elements.Num(); i++)
				{
					// Draw debug boxes around elements
					max = elements[i].BoxSphereBounds.BoxExtent.GetMax();
					maxExtent = FVector(max, max, max);
					center = elements[i].MyActor->GetActorLocation();

					DrawDebugBox(GetWorld(), center, maxExtent, FColor().Blue, false, 0.0f);
					DrawDebugSphere(GetWorld(), center + maxExtent, 4.0f, 12, FColor().White, false, 0.0f);
					DrawDebugSphere(GetWorld(), center - maxExtent, 4.0f, 12, FColor().White, false, 0.0f);
					elementCount++;
				}
			});

		//UE_LOG(LogTemp, Log, TEXT("Node Count: %d, Element Count: %d"), nodeCount, elementCount);
	}

	
}

void ASpacePartioner::AddOctreeElement(const FOctreeElement& inNewOctreeElement)
{
	check(bInitialized);
	OctreeData->AddElement(inNewOctreeElement);
	UE_LOG(LogTemp, Log, TEXT("Added element to Octree."));
}

TArray<FOctreeElement> ASpacePartioner::GetElementsWithinBounds(const FBoxSphereBounds& inBoundingBoxQuery)
{
	FBoxCenterAndExtent boundingBoxQuery = FBoxCenterAndExtent(inBoundingBoxQuery);
	return GetElementsWithinBounds(boundingBoxQuery);
}

TArray<FOctreeElement> ASpacePartioner::GetElementsWithinBounds(const FBoxCenterAndExtent& inBoundingBoxQuery)
{
	// Iterating over a region in the octree and storing the elements
	TArray<FOctreeElement> octreeElements;

	OctreeData->FindAllElements([&octreeElements](const FOctreeElement& octElement)
		{
			octreeElements.Add(octElement);
		});
	UE_LOG(LogTemp, Log, TEXT("octreeElements: %d"), octreeElements.Num());

	return octreeElements;
}

void ASpacePartioner::DrawOctreeBounds()
{
	FVector extent = this->OctreeData->GetRootBounds().Extent;
	
	float max = extent.GetMax();
	FVector maxExtent = FVector(max, max, max);
	FVector center = this->OctreeData->GetRootBounds().Center;

	DrawDebugBox(GetWorld(), center, maxExtent, FColor().Blue, false, 0.0f);
	DrawDebugSphere(GetWorld(), center + maxExtent, 4.0f, 12, FColor().White, false, 0.0f);
	DrawDebugSphere(GetWorld(), center - maxExtent, 4.0f, 12, FColor().White, false, 0.0f);
}