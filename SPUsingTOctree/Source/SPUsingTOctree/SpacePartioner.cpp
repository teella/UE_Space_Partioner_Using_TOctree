// Fill out your copyright notice in the Description page of Project Settings.

#include "SpacePartioner.h"
#include "SPUsingTOctree.h"
#include "Engine/Public/DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

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
void ASpacePartioner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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
	TQueue<FOctreeElement> octElements;
	FBox box = inBoundingBoxQuery.GetBox();
	FSphere sphere = inBoundingBoxQuery.GetSphere();
	FBox sphereBox = FBox(FVector(sphere.Center.X - sphere.W, sphere.Center.Y - sphere.W, sphere.Center.Z - sphere.W),
		FVector(sphere.Center.X + sphere.W, sphere.Center.Y + sphere.W, sphere.Center.Z + sphere.W));

	//--just for reference--
	//OctreeData->FindAllElements([&octreeElements, &box, &sphere](const FOctreeElement& octElement)
	//	{
	//		if (box.IsInside(octElement.BoxSphereBounds.GetBox()) || sphere.IsInside(octElement.MyActor->GetActorLocation()))
	//		{
	//			octreeElements.Add(octElement);
	//		}
	//	});

	OctreeData->FindNodesWithPredicate(
		[&box, &sphereBox](FSimpleOctree::FNodeIndex /*ParentNodeIndex*/, FSimpleOctree::FNodeIndex /*NodeIndex*/, const FBoxCenterAndExtent& NodeBounds)
		{
			if (NodeBounds.GetBox().IsInside(box.GetCenter()) || NodeBounds.GetBox().Intersect(box) || NodeBounds.GetBox().Intersect(sphereBox))
			{
				return true;
			}
			return false;
		},
		[this, &octreeElements, &box, &sphere, &bSphereOnlyTest, &octElements](FSimpleOctree::FNodeIndex /*ParentNodeIndex*/, FSimpleOctree::FNodeIndex NodeIndex, const FBoxCenterAndExtent& /*NodeBounds*/)
		{
			TArrayView<const FOctreeElement> elements = OctreeData->GetElementsForNode(NodeIndex);
			int numElements = elements.Num();
			if (PrintLogs) UE_LOG(LogTemp, Log, TEXT("GetElementsWithinBounds NodeIndex: %d numElements: %i"), NodeIndex, numElements);

			ParallelFor(numElements, [this, box, sphere, bSphereOnlyTest, elements, &octElements](int32 Index)
				{
					if (
						(!bSphereOnlyTest && (box.IsInside(elements[Index].BoxSphereBounds.GetBox()) || sphere.IsInside(elements[Index].MyActor->GetActorLocation())))
						||
						(bSphereOnlyTest && sphere.IsInside(elements[Index].MyActor->GetActorLocation()))
						)
					{
						octElements.Enqueue(elements[Index]);
					}
				});

			if (!octElements.IsEmpty())
			{
				FOctreeElement element;
				while (octElements.Dequeue(element))
				{
					octreeElements.Add(element);
				}
			}

			/*
			for (int i = 0; i < elements.Num(); i++)
			{
				if (
					(!bSphereOnlyTest && (box.IsInside(elements[i].BoxSphereBounds.GetBox()) || sphere.IsInside(elements[i].MyActor->GetActorLocation())))
					||
					(bSphereOnlyTest && sphere.IsInside(elements[i].MyActor->GetActorLocation()))
					)
				{
					octreeElements.Add(elements[i]);
				}
			}
			*/
		});

	if (PrintLogs) UE_LOG(LogTemp, Log, TEXT("GetElementsWithinBounds octreeElements: %d"), octreeElements.Num());

	return octreeElements;
}

void ASpacePartioner::DrawBoxSphereBounds(const FBoxSphereBounds& inBoundingBoxQuery, const bool bSphereOnlyTest = false, const bool bPersistentLines = false, const float lifeTime = 0.0f)
{
	DrawDebugBox(GetWorld(), inBoundingBoxQuery.Origin, inBoundingBoxQuery.BoxExtent, (bSphereOnlyTest ? FColor(128.0f, 128.0f, 128.0f, 128.0f) : FColor().Purple), bPersistentLines, lifeTime);
	DrawDebugSphere(GetWorld(), inBoundingBoxQuery.Origin, inBoundingBoxQuery.SphereRadius, 12, (bSphereOnlyTest ? FColor().Orange : FColor().Turquoise), bPersistentLines, lifeTime, 0, (bSphereOnlyTest ? 3.0f : 0.0f));
}

void ASpacePartioner::GetAllActorsWithinBounds(const FBoxSphereBounds& inBoundingBoxQuery, TSubclassOf<AActor> ActorClass, TArray<AActor*>& OutActors, const bool bSphereOnlyTest, const bool bDrawDebug, const bool bPersistentLines, const float lifeTime)
{
	if (bDrawDebug)
	{
		DrawDebugBox(GetWorld(), inBoundingBoxQuery.Origin, inBoundingBoxQuery.BoxExtent, (bSphereOnlyTest ? FColor(128.0f, 128.0f, 128.0f, 128.0f) : FColor().Red), bPersistentLines, lifeTime);
		DrawDebugSphere(GetWorld(), inBoundingBoxQuery.Origin, inBoundingBoxQuery.SphereRadius, 12, (bSphereOnlyTest ? FColor().Cyan : FColor().Red), bPersistentLines, lifeTime, 0, (bSphereOnlyTest ? 3.0f : 0.0f));
	}
	OutActors.Reset();

	FBox box = inBoundingBoxQuery.GetBox();
	FSphere sphere = inBoundingBoxQuery.GetSphere();
	FBox sphereBox = FBox(FVector(sphere.Center.X - sphere.W, sphere.Center.Y - sphere.W, sphere.Center.Z - sphere.W),
		FVector(sphere.Center.X + sphere.W, sphere.Center.Y + sphere.W, sphere.Center.Z + sphere.W));
	TQueue<AActor*> actorsQueue;
	TArray<AActor*> foundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ActorClass, foundActors);

	int numActors = foundActors.Num();
	if (PrintLogs) UE_LOG(LogTemp, Log, TEXT("GetAllActorsWithinBounds numActors: %d"), numActors);

	ParallelFor(numActors, [this, box, sphere, bSphereOnlyTest, foundActors, &actorsQueue](int32 Index)
		{
			FBox actorBounds = foundActors[Index]->GetComponentsBoundingBox(true, true);
			if (
				(!bSphereOnlyTest && (box.IsInside(actorBounds) || sphere.IsInside(foundActors[Index]->GetActorLocation())))
				||
				(bSphereOnlyTest && sphere.IsInside(foundActors[Index]->GetActorLocation()))
				)
			{
				actorsQueue.Enqueue(foundActors[Index]);
			}
		});

	//foundActors.RemoveAll([](AActor * actor) { return true; });

	if (!actorsQueue.IsEmpty())
	{
		AActor* actor;
		while (actorsQueue.Dequeue(actor))
		{
			OutActors.Add(actor);
		}
	}

	if (PrintLogs) UE_LOG(LogTemp, Log, TEXT("GetAllActorsWithinBounds OutActors: %d"), OutActors.Num());
}

void ASpacePartioner::AddActorToOctree(AActor* inActor)
{
	if (inActor)
	{
		FVector origin;
		FVector boxExtent;
		inActor->GetActorBounds(false, origin, boxExtent);
		double maxExtent = boxExtent.GetMax();

		if (maxExtent < OctreeData->GetRootBounds().GetBox().GetExtent().GetMax())
		{
			FOctreeElement element = FOctreeElement(inActor, FBoxSphereBounds(origin, boxExtent, maxExtent));
			check(bInitialized);
			OctreeData->AddElement(element);
		}
		else
		{
			//for things like skysphere
			if (PrintLogs) UE_LOG(LogTemp, Log, TEXT("AddActorToOctree: maxExtent greater than OctreeData->GetRootBounds maxExtent!"));
		}
	}
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
