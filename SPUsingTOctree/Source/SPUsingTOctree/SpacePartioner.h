// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Math/GenericOctree.h"

#include "GameFramework/Actor.h"
#include "SpacePartioner.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct FOctreeElement
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Octree Element Struct")
	TObjectPtr<AActor> MyActor = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Octree Element Struct")
	FBoxSphereBounds BoxSphereBounds;

	FOctreeElement()
	{
		BoxSphereBounds = FBoxSphereBounds(FVector(0.0f, 0.0f, 0.0f), FVector(1.0f, 1.0f, 1.0f), 1.0f);
	}

	FOctreeElement(AActor* inActor, FBoxSphereBounds inBoxSphereBounds)
	{
		MyActor = inActor;
		BoxSphereBounds = inBoxSphereBounds;
	}
};

struct FOctreeSematics
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
	FORCEINLINE static FBoxSphereBounds GetBoundingBox(const FOctreeElement& Element)
	{
		return Element.BoxSphereBounds;
	}

	FORCEINLINE static bool AreElementsEqual(const FOctreeElement& A, const FOctreeElement& B)
	{
		return A.MyActor == B.MyActor;
	}
	
	FORCEINLINE static void SetElementId(const FOctreeElement& Element, FOctreeElementId2 Id)
	{
	}

	FORCEINLINE static void ApplyOffset(FOctreeElement& Element, FVector Offset)
	{
		FVector NewPostion = Element.MyActor->GetActorLocation() + Offset;
		Element.MyActor->SetActorLocation(NewPostion);
		Element.BoxSphereBounds.Origin = NewPostion;
	}

};

typedef TOctree2<FOctreeElement, FOctreeSematics> FSimpleOctree;

UCLASS(ClassGroup = (SpacePartioner), BlueprintType, Blueprintable)
class SPUSINGTOCTREE_API ASpacePartioner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASpacePartioner(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	/**
	* Adds a spefic element to the Octree
	* @param NewOctreeElement	FOctreeElement to be added.
	*/
	UFUNCTION(BlueprintCallable, Category = Octree)
	void AddOctreeElement(const FOctreeElement& inNewOctreeElement);

	/**
	* Returns elements within the specified region.
	* @param inBoundingBoxQuery	Box to query Octree.
	* @return TArray of Elements
	*/
	UFUNCTION(BlueprintCallable, Category = Octree)
	TArray<FOctreeElement> GetElementsWithinBounds(const FBoxSphereBounds& inBoundingBoxQuery, const bool bSphereOnlyTest, const bool bDrawDebug, const bool bPersistentLines, const float lifeTime);
	
	/** Draws Debug information at runtime */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug)
	bool bDrawDebugInfo;

	UFUNCTION(BlueprintCallable, Category = Octree)
	void DrawBoxSphereBounds(const FBoxSphereBounds& inBoundingBoxQuery, const bool bSphereOnlyTest, const bool bPersistentLines, const float lifeTime);

	UFUNCTION(BlueprintCallable, Category = Octree)
	void GetAllActorsWithinBounds(const FBoxSphereBounds& inBoundingBoxQuery, TSubclassOf<AActor> ActorClass, TArray<AActor*>& OutActors, const bool bSphereOnlyTest, const bool bDrawDebug, const bool bPersistentLines, const float lifeTime);

	UFUNCTION(BlueprintCallable, Category = Octree)
	void AddActorToOctree(AActor* inActor);

private:

	void DrawOctreeBounds();

	TObjectPtr<FSimpleOctree> OctreeData = nullptr;
	bool bInitialized;
	
};

