// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/GenericOctree.h"
#include "SPOctree.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct FSPOctreeElement
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Octree Element Struct")
	TObjectPtr<AActor> MyActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Octree Element Struct")
	FBoxSphereBounds BoxSphereBounds;

	FSPOctreeElement()
	{
		BoxSphereBounds = FBoxSphereBounds(FVector(0.0f, 0.0f, 0.0f), FVector(1.0f, 1.0f, 1.0f), 1.0f);
	}

	FSPOctreeElement(AActor* inActor, FBoxSphereBounds inBoxSphereBounds)
	{
		MyActor = inActor;
		BoxSphereBounds = inBoxSphereBounds;
	}
};

struct FSPOctreeSematics
{
	enum { MaxElementsPerLeaf = 2 }; // 16
	enum { MinInclusiveElementsPerNode = 7 };
	enum { MaxNodeDepth = 12 };

	typedef TInlineAllocator<MaxElementsPerLeaf> ElementAllocator;

	/**
	* Get the bounding box of the provided octree element. In this case, the box
	* is merely the point specified by the element.
	*
	* @param	Element	Octree element to get the bounding box for
	*
	* @return	Bounding box of the provided octree element
	*/
	FORCEINLINE static FBoxSphereBounds GetBoundingBox(const FSPOctreeElement& Element)
	{
		return Element.BoxSphereBounds;
	}

	FORCEINLINE static bool AreElementsEqual(const FSPOctreeElement& A, const FSPOctreeElement& B)
	{
		return A.MyActor == B.MyActor;
	}

	FORCEINLINE static void SetElementId(const FSPOctreeElement& Element, FOctreeElementId2 Id)
	{
	}

	FORCEINLINE static void ApplyOffset(FSPOctreeElement& Element, FVector Offset)
	{
		FVector NewPostion = Element.MyActor->GetActorLocation() + Offset;
		Element.MyActor->SetActorLocation(NewPostion);
		Element.BoxSphereBounds.Origin = NewPostion;
	}

};

typedef TOctree2<FSPOctreeElement, FSPOctreeSematics> FSPOctree;

UCLASS(ClassGroup = (SPOctree), BlueprintType, Blueprintable)
class SPOCTREEDATALAYER_API ASPOctree : public AActor
{
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	ASPOctree(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(Category = "Config", EditAnywhere, BlueprintReadWrite)
	bool PrintLogs;

	UPROPERTY(Category = "Config", EditAnywhere, BlueprintReadWrite)
	bool PrintTickLogs;

	/**
	* Used in conjunction with a constructor to initialize the object.
	* @param NewBounds	Intial size of the Octree
	* @param inDrawDebugInfo	Whether or not to display debug boundaries
	*/
	UFUNCTION(BlueprintCallable, Category = Octree)
	void Initialize(const FBox& inNewBounds, const bool& inDrawDebugInfo);

	/**
	* Used in conjunction with a constructor to initialize the object.
	* @param inExtent	Intial size of the Octree
	* @param inDrawDebugInfo	Whether or not to display debug boundaries
	*/
	void Initialize(const float& inExtent, const bool& inDrawDebugInfo);

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	/**
	* Adds a spefic element to the Octree
	* @param NewOctreeElement	FSPOctreeElement to be added.
	*/
	UFUNCTION(BlueprintCallable, Category = Octree)
	void AddOctreeElement(const FSPOctreeElement& inNewOctreeElement);

	/**
	* Returns elements within the specified region.
	* @param inBoundingBoxQuery	Box to query Octree.
	* @return TArray of Elements
	*/
	UFUNCTION(BlueprintCallable, Category = Octree)
	TArray<FSPOctreeElement> GetElementsWithinBounds(const FBoxSphereBounds& inBoundingBoxQuery, const bool bSphereOnlyTest, const bool bDrawDebug, const bool bPersistentLines, const float lifeTime);

	/** Draws Debug information at runtime */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug)
	bool bDrawDebugInfo;

	UFUNCTION(BlueprintCallable, Category = Octree)
	void DrawBoxSphereBounds(const FBoxSphereBounds& inBoundingBoxQuery, const bool bSphereOnlyTest, const bool bPersistentLines, const float lifeTime);

	UFUNCTION(BlueprintCallable, Category = Octree)
	void GetAllActorsWithinBounds(const FBoxSphereBounds& inBoundingBoxQuery, TSubclassOf<AActor> ActorClass, TArray<AActor*>& OutActors, const bool bSphereOnlyTest, const bool bDrawDebug, const bool bPersistentLines, const float lifeTime);

	UFUNCTION(BlueprintCallable, Category = Octree)
	void AddActorToOctree(AActor* inActor);
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

	void DrawOctreeBounds();

	TObjectPtr<FSPOctree> OctreeData = nullptr;
	bool bInitialized;

};
