#pragma once

#include "Data.h"
#include "MetaModelBase.h"

#include "Constants/GlobalConstants.h"

#include "MetaConfigItemTypes.generated.h"

enum class EItemType : uint8;
enum class EItemTier : uint8;

UCLASS(Within = MetaConfig)
class ASYNCCUSTOMISATION_API UMetaConfigItemData : public UMetaModelBase
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EItemType Type = EItemType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EItemTier Tier = EItemTier::None;
};


USTRUCT(BlueprintType)
struct FProductItem
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	const UMetaConfigItemData* Item = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Amount = 0;
};



//namespace fmt
//{
//	template <> struct formatter<UMetaConfigItemData, TCHAR> : RF::EmptyFmtParse
//	{
//		template <typename FormatContext> auto format(const UMetaConfigItemData& ConfigItem, FormatContext& Context) const
//		{
//			return format_to(Context.out(), TEXT("(id: {}, slug: {})"), ConfigItem.ServerId, ConfigItem.GameId);
//		}
//	};
//} // namespace fmt
