// Fill out your copyright notice in the Description page of Project Settings.


#include "SPOctreeStreamingSourceComponent.h"
#include "SPOctreeDataLayer.h"
#include "SPOctree.h"

// Sets default values for this component's properties
USPOctreeStreamingSourceComponent::USPOctreeStreamingSourceComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
	
	PrintLogs = false;
	DistanceCheck = 1500;
	PrintLogsInterval = 1;
}


// Called when the game starts
void USPOctreeStreamingSourceComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// ...
	
}

void USPOctreeStreamingSourceComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		Owner = GetOwner<AActor>();
		if (Owner == nullptr)
		{
			UE_LOG(SPOctreeDataLayerMod, Error, TEXT("USPOctreeStreamingSourceComponent: !!!!!!!!!!!!!!!! Failed to get Owner !!!!!!!!!!!!!!!!"));
		}
	}

}

void USPOctreeStreamingSourceComponent::UninitializeComponent()
{
	if (PrintLogs) UE_LOG(SPOctreeDataLayerMod, Log, TEXT("USPOctreeStreamingSourceComponent: UninitializeComponent"));

	Super::UninitializeComponent();
}
// Called every frame
void USPOctreeStreamingSourceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetWorld() && GetWorld()->IsGameWorld() && Owner)
	{
		if (!octrees.IsEmpty())
		{
			float worldTime = GetWorld()->GetTimeSeconds();
			FVector origin;
			FVector boxExtent;
			Owner->GetActorBounds(false, origin, boxExtent);
			FBoxSphereBounds ownerBounds = FBoxSphereBounds(origin, boxExtent, DistanceCheck);
			TArray<FSPOctreeElement> foundElements;

			for (int index = 0; index < octrees.Num(); index++)
			{
				foundElements = octrees[index]->GetElementsWithinBounds(ownerBounds, true, DrawDebug, false, 0);

				if (PrintLogs && nextPrintLogTime < worldTime) UE_LOG(SPOctreeDataLayerMod, Log, TEXT("USPOctreeStreamingSourceComponent: foundElements: %i"), foundElements.Num());

				for (int found = 0; found < foundElements.Num(); found++)
				{
					if (foundElements[found].MyActor)
					{
						foundElements[found].MyActor->SetActorHiddenInGame(false);
						foundElements[found].MyActor->SetActorTickEnabled(foundElements[found].MyActor->PrimaryActorTick.bStartWithTickEnabled);
						if (trackedElements.Find(foundElements[found]) == INDEX_NONE)
						{
							trackedElements.Add(foundElements[found]);
						}
					}
				}

				if (PrintLogs && nextPrintLogTime < worldTime) UE_LOG(SPOctreeDataLayerMod, Log, TEXT("USPOctreeStreamingSourceComponent: Current trackedElements: %i"), trackedElements.Num());

				for (int i = 0; i < trackedElements.Num(); i++)
				{
					if (!trackedElements[i].MyActor || foundElements.Find(trackedElements[i]) == INDEX_NONE)
					{
						if (trackedElements[i].MyActor)
						{
							trackedElements[i].MyActor->SetActorHiddenInGame(true);
							trackedElements[i].MyActor->SetActorTickEnabled(false);
						}
						trackedElements.RemoveAt(i);
						i--;
					}
				}

				if (PrintLogs && nextPrintLogTime < worldTime) UE_LOG(SPOctreeDataLayerMod, Log, TEXT("USPOctreeStreamingSourceComponent: Still in range trackedElements: %i"), trackedElements.Num());
			}

			if (PrintLogs && nextPrintLogTime < worldTime)
			{
				nextPrintLogTime = GetWorld()->GetTimeSeconds() + 1;
			}
		}
	}
}

void USPOctreeStreamingSourceComponent::addOctree(ASPOctree* inOctree)
{
	octrees.Add(inOctree);
}

void USPOctreeStreamingSourceComponent::removeOctree(ASPOctree* inOctree)
{
	octrees.Remove(inOctree);
}
