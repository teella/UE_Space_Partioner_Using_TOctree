// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SPOctree.h"
#include "SPOctreeStreamingSourceComponent.generated.h"


UCLASS( ClassGroup = (SPOctree), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent) )
class SPOCTREEDATALAYER_API USPOctreeStreamingSourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USPOctreeStreamingSourceComponent();

	UPROPERTY(Category = "Config", EditAnywhere, BlueprintReadWrite)
	bool DrawDebug;

	UPROPERTY(Category = "Config", EditAnywhere, BlueprintReadWrite)
	bool PrintLogs;

	UPROPERTY(Category = "Config", EditAnywhere, BlueprintReadWrite)
	float PrintLogsInterval;

	UPROPERTY(Category = "Config", EditAnywhere, BlueprintReadWrite)
	float DistanceCheck;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void InitializeComponent() override;

	virtual void UninitializeComponent() override;

	UFUNCTION(BlueprintCallable, Category = Octree)
	void addOctree(ASPOctree * inOctree);

	UFUNCTION(BlueprintCallable, Category = Octree)
	void removeOctree(ASPOctree* inOctree);

private:
	TObjectPtr<AActor> Owner = nullptr;

	TArray<TObjectPtr<ASPOctree>> octrees;
	TArray<FSPOctreeElement> trackedElements;

	float nextPrintLogTime;
};
