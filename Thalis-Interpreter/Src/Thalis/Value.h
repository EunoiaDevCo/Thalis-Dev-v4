#pragma once

#include "Common.h"
#include "Memory/Allocator.h"
#include <iostream>

#define POINTER_LEVEL_REFERENCE UINT8_MAX
#define MAX_ARRAY_DIMENSIONS 8

struct ArrayHeader
{
	uint8 elementPointerLevel;
	uint8 numDimensions;
	uint32 dimensions[MAX_ARRAY_DIMENSIONS];
};

enum class ValueType
{
	FIRST_TYPE,
	UINT8, UINT16, UINT32, UINT64,
	INT8, INT16, INT32, INT64,
	REAL32, REAL64,
	BOOL, CHAR,
	VOID_T,
	TEMPLATE_TYPE,
	LAST_TYPE
};

class Program;
struct Value
{
	uint16 type;
	uint8 pointerLevel;
	bool isArray;
	bool isReference;
	void* data;

	inline bool IsInteger() const
	{
		switch ((ValueType)type)
		{
		case ValueType::UINT8:
		case ValueType::UINT16:
		case ValueType::UINT32:
		case ValueType::UINT64:
		case ValueType::INT8:
		case ValueType::INT16:
		case ValueType::INT32:
		case ValueType::INT64:
			return true;
		default:
			return false;
		}
	}

	inline bool IsUnsigned() const
	{
		switch ((ValueType)type)
		{
		case ValueType::UINT8:
		case ValueType::UINT16:
		case ValueType::UINT32:
		case ValueType::UINT64:
			return true;
		default:
			return false;
		}
	}

	inline bool IsSigned() const
	{
		switch ((ValueType)type)
		{
		case ValueType::INT8:
		case ValueType::INT16:
		case ValueType::INT32:
		case ValueType::INT64:
			return true;
		default:
			return false;
		}
	}

	inline bool IsReal() const { return type == (uint16)ValueType::REAL32 || type == (uint16)ValueType::REAL64; }
	inline bool IsBool() const { return type == (uint16)ValueType::BOOL; }
	inline bool IsChar() const { return type == (uint16)ValueType::CHAR; }
	inline bool IsPointer() const { return pointerLevel > 0; }
	inline bool IsPrimitive() const { return (type > (uint16)ValueType::FIRST_TYPE) && (type < (uint16)ValueType::LAST_TYPE); }

	inline uint32 GetBitWidth() const
	{
		switch ((ValueType)type)
		{
		case ValueType::UINT8:
		case ValueType::INT8: return 8;
		case ValueType::UINT16:
		case ValueType::INT16: return 16;
		case ValueType::UINT32:
		case ValueType::INT32:
		case ValueType::REAL32: return 32;
		case ValueType::UINT64:
		case ValueType::INT64:
		case ValueType::REAL64: return 64;
		case ValueType::BOOL: return 1;
		case ValueType::CHAR: return 8;
		default: return 0;
		}
	}

	inline uint8 GetUInt8() const
	{
		return GetUInt64();
	}

	inline uint16 GetUInt16() const
	{
		return GetUInt64();
	}

	inline uint32 GetUInt32() const
	{
		return GetUInt64();
	}

	inline uint64 GetUInt64() const
	{
		switch ((ValueType)type)
		{
		case ValueType::UINT8: return *(uint8*)data;
		case ValueType::UINT16: return *(uint16*)data;
		case ValueType::UINT32: return *(uint32*)data;
		case ValueType::UINT64: return *(uint64*)data;
		case ValueType::INT8: return *(int8*)data;
		case ValueType::INT16: return *(int16*)data;
		case ValueType::INT32: return *(int32*)data;
		case ValueType::INT64: return *(int64*)data;
		case ValueType::REAL32: return *(real32*)data;
		case ValueType::REAL64: return *(real64*)data;
		case ValueType::BOOL: return *(bool*)data;
		case ValueType::CHAR: return *(char*)data;
		}

		return 0;
	}

	inline int8 GetInt8() const
	{
		return GetInt64();
	}

	inline int16 GetInt16() const
	{
		return GetInt64();
	}

	inline int32 GetInt32() const
	{
		return GetInt64();
	}

	inline int64 GetInt64() const
	{
		switch ((ValueType)type)
		{
		case ValueType::UINT8: return *(uint8*)data;
		case ValueType::UINT16: return *(uint16*)data;
		case ValueType::UINT32: return *(uint32*)data;
		case ValueType::UINT64: return *(uint64*)data;
		case ValueType::INT8: return *(int8*)data;
		case ValueType::INT16: return *(int16*)data;
		case ValueType::INT32: return *(int32*)data;
		case ValueType::INT64: return *(int64*)data;
		case ValueType::REAL32: return *(real32*)data;
		case ValueType::REAL64: return *(real64*)data;
		case ValueType::BOOL: return *(bool*)data;
		case ValueType::CHAR: return *(char*)data;
		}

		return 0;
	}

	inline real32 GetReal32() const
	{
		return GetReal64();
	}

	inline real64 GetReal64() const
	{
		switch ((ValueType)type)
		{
		case ValueType::UINT8: return *(uint8*)data;
		case ValueType::UINT16: return *(uint16*)data;
		case ValueType::UINT32: return *(uint32*)data;
		case ValueType::UINT64: return *(uint64*)data;
		case ValueType::INT8: return *(int8*)data;
		case ValueType::INT16: return *(int16*)data;
		case ValueType::INT32: return *(int32*)data;
		case ValueType::INT64: return *(int64*)data;
		case ValueType::REAL32: return *(real32*)data;
		case ValueType::REAL64: return *(real64*)data;
		case ValueType::BOOL: return *(bool*)data;
		case ValueType::CHAR: return *(char*)data;
		}

		return 0.0f;
	}

	inline char GetChar() const
	{
		return GetInt8();
	}

	inline bool GetBool() const
	{
		switch ((ValueType)type)
		{
		case ValueType::UINT8: return *(uint8*)data != 0;
		case ValueType::UINT16: return *(uint16*)data != 0;
		case ValueType::UINT32: return *(uint32*)data != 0;
		case ValueType::UINT64: return *(uint64*)data != 0;
		case ValueType::INT8: return *(int8*)data != 0;
		case ValueType::INT16: return *(int16*)data != 0;
		case ValueType::INT32: return *(int32*)data != 0;
		case ValueType::INT64: return *(int64*)data != 0;
		case ValueType::REAL32: return *(real32*)data != 0.0f;
		case ValueType::REAL64: return *(real64*)data != 0.0;
		case ValueType::BOOL: return *(bool*)data;
		case ValueType::CHAR: return *(char*)data != 0;
		}

		return false;
	}

	inline friend std::ostream& operator<<(std::ostream& os, const Value& v)
	{
		if (v.pointerLevel == 1 && v.type == (uint16)ValueType::CHAR)
		{
			os << *(char**)v.data;
			return os;
		}

		switch ((ValueType)v.type)
		{
		case ValueType::UINT8:   os << v.GetUInt8(); break;
		case ValueType::UINT16:  os << v.GetUInt16(); break;
		case ValueType::UINT32:  os << v.GetUInt32(); break;
		case ValueType::UINT64:  os << v.GetUInt64(); break;
		case ValueType::INT8:    os << static_cast<int>(v.GetInt8()); break; //Cast so it doesnt print a char
		case ValueType::INT16:   os << v.GetInt16(); break;
		case ValueType::INT32:   os << v.GetInt32(); break;
		case ValueType::INT64:   os << v.GetInt64(); break;
		case ValueType::REAL32:  os << v.GetReal32(); break;
		case ValueType::REAL64:  os << v.GetReal64(); break;
		case ValueType::BOOL:    os << (v.GetBool() ? "true" : "false"); break;
		case ValueType::CHAR:    os << v.GetChar(); break;
		}
		return os;
	}

	inline Value Add(const Value& rhs, Allocator* allocator)
	{
		Value result;

		if (IsInteger() && rhs.IsInteger())
		{
			uint32 lhsBits = GetBitWidth();
			uint32 rhsBits = rhs.GetBitWidth();
			uint32 maxBits = std::max(lhsBits, rhsBits);

			bool lhsSigned = IsSigned();
			bool rhsSigned = rhs.IsSigned();

			if (lhsSigned || rhsSigned)
			{
				if (maxBits <= 8)  return MakeInt8((int8)(GetInt64() + rhs.GetInt64()), allocator);
				if (maxBits <= 16) return MakeInt16((int16)(GetInt64() + rhs.GetInt64()), allocator);
				if (maxBits <= 32) return MakeInt32((int32)(GetInt64() + rhs.GetInt64()), allocator);
				return MakeInt64(GetInt64() + rhs.GetInt64(), allocator);
			}
			else
			{
				if (maxBits <= 8)  return MakeUInt8((uint8)(GetUInt64() + rhs.GetUInt64()), allocator);
				if (maxBits <= 16) return MakeUInt16((uint16)(GetUInt64() + rhs.GetUInt64()), allocator);
				if (maxBits <= 32) return MakeUInt32((uint32)(GetUInt64() + rhs.GetUInt64()), allocator);
				return MakeUInt64(GetUInt64() + rhs.GetUInt64(), allocator);
			}
		}

		if (IsReal() || rhs.IsReal())
		{
			if (type == (uint16)ValueType::REAL64 || rhs.type == (uint16)ValueType::REAL64)
				return MakeReal64(GetReal64() + rhs.GetReal64(), allocator);
			else
				return MakeReal32((real32)(GetReal32() + rhs.GetReal32()), allocator);
		}

		return MakeInt64(GetInt64() + rhs.GetInt64(), allocator);
	}

	inline Value Sub(const Value& rhs, Allocator* allocator) const
	{
		Value result;

		if (IsReal() || rhs.IsReal())
		{
			if (type == (uint16)ValueType::REAL64 || rhs.type == (uint16)ValueType::REAL64)
				result = Value::MakeReal64(GetReal64() - rhs.GetReal64(), allocator);
			else
				result = Value::MakeReal32((real32)(GetReal32() - rhs.GetReal32()), allocator);
			return result;
		}

		if (IsInteger() && rhs.IsInteger())
		{
			uint32 lhsBits = GetBitWidth();
			uint32 rhsBits = rhs.GetBitWidth();
			uint32 maxBits = std::max(lhsBits, rhsBits);

			bool lhsSigned = IsSigned();
			bool rhsSigned = rhs.IsSigned();

			if (lhsSigned || rhsSigned)
			{
				if (maxBits <= 8)  return Value::MakeInt8((int8)(GetInt64() - rhs.GetInt64()), allocator);
				if (maxBits <= 16) return Value::MakeInt16((int16)(GetInt64() - rhs.GetInt64()), allocator);
				if (maxBits <= 32) return Value::MakeInt32((int32)(GetInt64() - rhs.GetInt64()), allocator);
				return Value::MakeInt64(GetInt64() - rhs.GetInt64(), allocator);
			}
			else
			{
				if (maxBits <= 8)  return Value::MakeUInt8((uint8)(GetUInt64() - rhs.GetUInt64()), allocator);
				if (maxBits <= 16) return Value::MakeUInt16((uint16)(GetUInt64() - rhs.GetUInt64()), allocator);
				if (maxBits <= 32) return Value::MakeUInt32((uint32)(GetUInt64() - rhs.GetUInt64()), allocator);
				return Value::MakeUInt64(GetUInt64() - rhs.GetUInt64(), allocator);
			}
		}

		return Value::MakeInt64(GetInt64() - rhs.GetInt64(), allocator);
	}

	inline Value Mul(const Value& rhs, Allocator* allocator) const
	{
		Value result;

		if (IsReal() || rhs.IsReal())
		{
			if (type == (uint16)ValueType::REAL64 || rhs.type == (uint16)ValueType::REAL64)
				result = Value::MakeReal64(GetReal64() * rhs.GetReal64(), allocator);
			else
				result = Value::MakeReal32((real32)(GetReal32() * rhs.GetReal32()), allocator);
			return result;
		}

		if (IsInteger() && rhs.IsInteger())
		{
			uint32 lhsBits = GetBitWidth();
			uint32 rhsBits = rhs.GetBitWidth();
			uint32 maxBits = std::max(lhsBits, rhsBits);

			bool lhsSigned = IsSigned();
			bool rhsSigned = rhs.IsSigned();

			if (lhsSigned || rhsSigned)
			{
				if (maxBits <= 8)  return Value::MakeInt8((int8)(GetInt64() * rhs.GetInt64()), allocator);
				if (maxBits <= 16) return Value::MakeInt16((int16)(GetInt64() * rhs.GetInt64()), allocator);
				if (maxBits <= 32) return Value::MakeInt32((int32)(GetInt64() * rhs.GetInt64()), allocator);
				return Value::MakeInt64(GetInt64() * rhs.GetInt64(), allocator);
			}
			else
			{
				if (maxBits <= 8)  return Value::MakeUInt8((uint8)(GetUInt64() * rhs.GetUInt64()), allocator);
				if (maxBits <= 16) return Value::MakeUInt16((uint16)(GetUInt64() * rhs.GetUInt64()), allocator);
				if (maxBits <= 32) return Value::MakeUInt32((uint32)(GetUInt64() * rhs.GetUInt64()), allocator);
				return Value::MakeUInt64(GetUInt64() * rhs.GetUInt64(), allocator);
			}
		}

		return Value::MakeInt64(GetInt64() * rhs.GetInt64(), allocator);
	}

	inline Value Div(const Value& rhs, Allocator* allocator) const
	{
		Value result;

		if (IsReal() || rhs.IsReal())
		{
			if (type == (uint16)ValueType::REAL64 || rhs.type == (uint16)ValueType::REAL64)
				result = Value::MakeReal64(GetReal64() / rhs.GetReal64(), allocator);
			else
				result = Value::MakeReal32((real32)(GetReal32() / rhs.GetReal32()), allocator);
			return result;
		}

		if (IsInteger() && rhs.IsInteger())
		{
			uint32 lhsBits = GetBitWidth();
			uint32 rhsBits = rhs.GetBitWidth();
			uint32 maxBits = std::max(lhsBits, rhsBits);

			bool lhsSigned = IsSigned();
			bool rhsSigned = rhs.IsSigned();

			if (lhsSigned || rhsSigned) 
			{
				if (maxBits <= 8)  return Value::MakeInt8((int8)(GetInt64() / rhs.GetInt64()), allocator);
				if (maxBits <= 16) return Value::MakeInt16((int16)(GetInt64() / rhs.GetInt64()), allocator);
				if (maxBits <= 32) return Value::MakeInt32((int32)(GetInt64() / rhs.GetInt64()), allocator);
				return Value::MakeInt64(GetInt64() / rhs.GetInt64(), allocator);
			}
			else 
			{
				if (maxBits <= 8)  return Value::MakeUInt8((uint8)(GetUInt64() / rhs.GetUInt64()), allocator);
				if (maxBits <= 16) return Value::MakeUInt16((uint16)(GetUInt64() / rhs.GetUInt64()), allocator);
				if (maxBits <= 32) return Value::MakeUInt32((uint32)(GetUInt64() / rhs.GetUInt64()), allocator);
				return Value::MakeUInt64(GetUInt64() / rhs.GetUInt64(), allocator);
			}
		}

		return Value::MakeInt64(GetInt64() / rhs.GetInt64(), allocator);
	}

	inline Value Mod(const Value& rhs, Allocator* allocator)
	{
		if (IsReal() || rhs.IsReal())
		{
			return Value::MakeNULL();
		}

		Value result;
		if (IsInteger() && rhs.IsInteger())
		{
			uint32 lhsBits = GetBitWidth();
			uint32 rhsBits = rhs.GetBitWidth();
			uint32 maxBits = std::max(lhsBits, rhsBits);

			bool lhsSigned = IsSigned();
			bool rhsSigned = rhs.IsSigned();

			if (lhsSigned || rhsSigned)
			{
				if (maxBits <= 8)  return Value::MakeInt8((int8)(GetInt64() % rhs.GetInt64()), allocator);
				if (maxBits <= 16) return Value::MakeInt16((int16)(GetInt64() % rhs.GetInt64()), allocator);
				if (maxBits <= 32) return Value::MakeInt32((int32)(GetInt64() % rhs.GetInt64()), allocator);
				return Value::MakeInt64(GetInt64() / rhs.GetInt64(), allocator);
			}
			else 
			{
				if (maxBits <= 8)  return Value::MakeUInt8((uint8)(GetUInt64() % rhs.GetUInt64()), allocator);
				if (maxBits <= 16) return Value::MakeUInt16((uint16)(GetUInt64() % rhs.GetUInt64()), allocator);
				if (maxBits <= 32) return Value::MakeUInt32((uint32)(GetUInt64() % rhs.GetUInt64()), allocator);
				return Value::MakeUInt64(GetUInt64() % rhs.GetUInt64(), allocator);
			}
		}

		return Value::MakeInt64(GetInt64() % rhs.GetInt64(), allocator);
	}

	inline Value LessThan(const Value& rhs, Allocator* allocator)
	{
		if (IsInteger() && rhs.IsInteger())
		{
			bool lhsSigned = IsSigned();
			bool rhsSigned = rhs.IsSigned();

			if (lhsSigned || rhsSigned)
			{
				bool result = GetInt64() < rhs.GetInt64();
				return MakeBool(result, allocator);
			}
			else
			{
				bool result = GetUInt64() < rhs.GetUInt64();
				return MakeBool(result, allocator);
			}
		}

		if (IsReal() || rhs.IsReal())
		{
			bool result = GetReal64() < rhs.GetReal64();
			return MakeBool(result, allocator);
		}

		bool result = GetInt64() < rhs.GetInt64();
		return MakeBool(result, allocator);
	}

	inline Value GreaterThan(const Value& rhs, Allocator* allocator)
	{
		if (IsInteger() && rhs.IsInteger())
		{
			bool lhsSigned = IsSigned();
			bool rhsSigned = rhs.IsSigned();

			if (lhsSigned || rhsSigned)
			{
				bool result = GetInt64() > rhs.GetInt64();
				return MakeBool(result, allocator);
			}
			else
			{
				bool result = GetUInt64() > rhs.GetUInt64();
				return MakeBool(result, allocator);
			}
		}

		if (IsReal() || rhs.IsReal())
		{
			bool result = GetReal64() > rhs.GetReal64();
			return MakeBool(result, allocator);
		}

		bool result = GetInt64() > rhs.GetInt64();
		return MakeBool(result, allocator);
	}

	inline Value LessThanOrEqual(const Value& rhs, Allocator* allocator)
	{
		if (IsInteger() && rhs.IsInteger())
		{
			bool lhsSigned = IsSigned();
			bool rhsSigned = rhs.IsSigned();

			if (lhsSigned || rhsSigned)
			{
				bool result = GetInt64() <= rhs.GetInt64();
				return MakeBool(result, allocator);
			}
			else
			{
				bool result = GetUInt64() <= rhs.GetUInt64();
				return MakeBool(result, allocator);
			}
		}

		if (IsReal() || rhs.IsReal())
		{
			bool result = GetReal64() <= rhs.GetReal64();
			return MakeBool(result, allocator);
		}

		bool result = GetInt64() <= rhs.GetInt64();
		return MakeBool(result, allocator);
	}

	inline Value GreaterThanOrEqual(const Value& rhs, Allocator* allocator)
	{
		if (IsInteger() && rhs.IsInteger())
		{
			bool lhsSigned = IsSigned();
			bool rhsSigned = rhs.IsSigned();

			if (lhsSigned || rhsSigned)
			{
				bool result = GetInt64() >= rhs.GetInt64();
				return MakeBool(result, allocator);
			}
			else
			{
				bool result = GetUInt64() >= rhs.GetUInt64();
				return MakeBool(result, allocator);
			}
		}

		if (IsReal() || rhs.IsReal())
		{
			bool result = GetReal64() >= rhs.GetReal64();
			return MakeBool(result, allocator);
		}

		bool result = GetInt64() >= rhs.GetInt64();
		return MakeBool(result, allocator);
	}

	inline Value Equals(const Value& rhs, Allocator* allocator)
	{
		if (IsPointer() && rhs.IsPointer())
		{
			if (pointerLevel != rhs.pointerLevel) return Value::MakeBool(false, allocator);
			return Value::MakeBool(data == rhs.data, allocator);
		}

		if (IsInteger() && rhs.IsInteger())
		{
			bool lhsSigned = IsSigned();
			bool rhsSigned = rhs.IsSigned();

			if (lhsSigned || rhsSigned)
			{
				bool result = GetInt64() == rhs.GetInt64();
				return MakeBool(result, allocator);
			}
			else
			{
				bool result = GetUInt64() == rhs.GetUInt64();
				return MakeBool(result, allocator);
			}
		}

		if (IsReal() || rhs.IsReal())
		{
			bool result = GetReal64() == rhs.GetReal64();
			return MakeBool(result, allocator);
		}

		bool result = GetInt64() == rhs.GetInt64();
		return MakeBool(result, allocator);
	}

	inline Value NotEquals(const Value& rhs, Allocator* allocator)
	{
		if (IsInteger() && rhs.IsInteger())
		{
			bool lhsSigned = IsSigned();
			bool rhsSigned = rhs.IsSigned();

			if (lhsSigned || rhsSigned)
			{
				bool result = GetInt64() != rhs.GetInt64();
				return MakeBool(result, allocator);
			}
			else
			{
				bool result = GetUInt64() != rhs.GetUInt64();
				return MakeBool(result, allocator);
			}
		}

		if (IsReal() || rhs.IsReal())
		{
			bool result = GetReal64() != rhs.GetReal64();
			return MakeBool(result, allocator);
		}

		bool result = GetInt64() != rhs.GetInt64();
		return MakeBool(result, allocator);
	}

	inline Value LogicalAnd(const Value& rhs, Allocator* allocator)
	{
		bool result = GetBool() && rhs.GetBool();
		return MakeBool(result, allocator);
	}

	inline Value LogicalOr(const Value& rhs, Allocator* allocator)
	{
		bool result = GetBool() || rhs.GetBool();
		return MakeBool(result, allocator);
	}

	inline void Increment()
	{
		switch ((ValueType)type)
		{
		case ValueType::UINT8:   (*(uint8*)data)++; break;
		case ValueType::UINT16:  (*(uint16*)data)++; break;
		case ValueType::UINT32:  (*(uint32*)data)++; break;
		case ValueType::UINT64:  (*(uint64*)data)++; break;
		case ValueType::INT8:    (*(int8*)data)++; break;
		case ValueType::INT16:   (*(int16*)data)++; break;
		case ValueType::INT32:   (*(int32*)data)++; break;
		case ValueType::INT64:   (*(int64*)data)++; break;
		case ValueType::REAL32:  (*(real32*)data)++; break;
		case ValueType::REAL64:  (*(real64*)data)++; break;
		case ValueType::CHAR:    (*(char*)data)++; break;
		}
	}

	inline void Decrement()
	{
		switch ((ValueType)type)
		{
		case ValueType::UINT8:   (*(uint8*)data)--; break;
		case ValueType::UINT16:  (*(uint16*)data)--; break;
		case ValueType::UINT32:  (*(uint32*)data)--; break;
		case ValueType::UINT64:  (*(uint64*)data)--; break;
		case ValueType::INT8:    (*(int8*)data)--; break;
		case ValueType::INT16:   (*(int16*)data)--; break;
		case ValueType::INT32:   (*(int32*)data)--; break;
		case ValueType::INT64:   (*(int64*)data)--; break;
		case ValueType::REAL32:  (*(real32*)data)--; break;
		case ValueType::REAL64:  (*(real64*)data)--; break;
		case ValueType::CHAR:    (*(char*)data)--; break;
		}
	}

	inline Value Invert(Allocator* allocator)
	{
		switch ((ValueType)type)
		{
		case ValueType::UINT8:   return Value::MakeBool(!(*(uint8*)data), allocator); break;
		case ValueType::UINT16:  return Value::MakeBool(!(*(uint16*)data), allocator); break;
		case ValueType::UINT32:  return Value::MakeBool(!(*(uint32*)data), allocator); break;
		case ValueType::UINT64:  return Value::MakeBool(!(*(uint64*)data), allocator); break;
		case ValueType::INT8:    return Value::MakeBool(!(*(int8*)data), allocator); break;
		case ValueType::INT16:   return Value::MakeBool(!(*(int16*)data), allocator); break;
		case ValueType::INT32:   return Value::MakeBool(!(*(int32*)data), allocator); break;
		case ValueType::INT64:   return Value::MakeBool(!(*(int64*)data), allocator); break;
		case ValueType::REAL32:  return Value::MakeBool(!(*(real32*)data), allocator); break;
		case ValueType::REAL64:  return Value::MakeBool(!(*(real64*)data), allocator); break;
		case ValueType::CHAR:    return Value::MakeBool(!(*(char*)data), allocator); break;
		case ValueType::BOOL:	 return Value::MakeBool(!(*(bool*)data), allocator); break;
		}

		return Value::MakeBool(false, allocator);
	}

	inline Value Negate(Allocator* allocator)
	{
		switch ((ValueType)type)
		{
		case ValueType::UINT8:   return Value::MakeUInt8(-(*(uint8*)data), allocator); break;
		case ValueType::UINT16:  return Value::MakeUInt16(-(*(uint16*)data), allocator); break;
		case ValueType::UINT32:  return Value::MakeUInt32(-(*(uint32*)data), allocator); break;
		case ValueType::UINT64:  return Value::MakeUInt64(-(*(uint64*)data), allocator); break;
		case ValueType::INT8:    return Value::MakeInt8(-(*(int8*)data), allocator); break;
		case ValueType::INT16:   return Value::MakeInt16(-(*(int16*)data), allocator); break;
		case ValueType::INT32:   return Value::MakeInt32(-(*(int32*)data), allocator); break;
		case ValueType::INT64:   return Value::MakeInt64(-(*(int64*)data), allocator); break;
		case ValueType::REAL32:  return Value::MakeReal32(-(*(real32*)data), allocator); break;
		case ValueType::REAL64:  return Value::MakeReal64(-(*(real64*)data), allocator); break;
		case ValueType::CHAR:    return Value::MakeChar(-(*(char*)data), allocator); break;
		case ValueType::BOOL:	 return Value::MakeBool(-(*(bool*)data), allocator); break;
		}

		return Value::MakeNULL();
	}

	inline void PlusEquals(const Value& value)
	{
		switch ((ValueType)type)
		{
		case ValueType::UINT8:   *(uint8*)data += value.GetUInt8(); break;
		case ValueType::UINT16:  *(uint16*)data += value.GetUInt16(); break;
		case ValueType::UINT32:  *(uint32*)data += value.GetUInt32(); break;
		case ValueType::UINT64:  *(uint64*)data += value.GetUInt64(); break;
		case ValueType::INT8:    *(int8*)data += value.GetInt8(); break;
		case ValueType::INT16:   *(int16*)data += value.GetInt16(); break;
		case ValueType::INT32:   *(int32*)data += value.GetInt32(); break;
		case ValueType::INT64:   *(int64*)data += value.GetInt64(); break;
		case ValueType::REAL32:  *(real32*)data += value.GetReal32(); break;
		case ValueType::REAL64:  *(real64*)data += value.GetReal64(); break;
		case ValueType::CHAR:    *(char*)data += value.GetChar(); break;
		case ValueType::BOOL:	 *(bool*)data += value.GetBool(); break;
		}
	}

	inline void MinusEquals(const Value& value)
	{
		switch ((ValueType)type)
		{
		case ValueType::UINT8:   *(uint8*)data -= value.GetUInt8(); break;
		case ValueType::UINT16:  *(uint16*)data -= value.GetUInt16(); break;
		case ValueType::UINT32:  *(uint32*)data -= value.GetUInt32(); break;
		case ValueType::UINT64:  *(uint64*)data -= value.GetUInt64(); break;
		case ValueType::INT8:    *(int8*)data -= value.GetInt8(); break;
		case ValueType::INT16:   *(int16*)data -= value.GetInt16(); break;
		case ValueType::INT32:   *(int32*)data -= value.GetInt32(); break;
		case ValueType::INT64:   *(int64*)data -= value.GetInt64(); break;
		case ValueType::REAL32:  *(real32*)data -= value.GetReal32(); break;
		case ValueType::REAL64:  *(real64*)data -= value.GetReal64(); break;
		case ValueType::CHAR:    *(char*)data -= value.GetChar(); break;
		case ValueType::BOOL:	 *(bool*)data -= value.GetBool(); break;
		}
	}

	inline void TimesEquals(const Value& value)
	{
		switch ((ValueType)type)
		{
		case ValueType::UINT8:   *(uint8*)data *= value.GetUInt8(); break;
		case ValueType::UINT16:  *(uint16*)data *= value.GetUInt16(); break;
		case ValueType::UINT32:  *(uint32*)data *= value.GetUInt32(); break;
		case ValueType::UINT64:  *(uint64*)data *= value.GetUInt64(); break;
		case ValueType::INT8:    *(int8*)data *= value.GetInt8(); break;
		case ValueType::INT16:   *(int16*)data *= value.GetInt16(); break;
		case ValueType::INT32:   *(int32*)data *= value.GetInt32(); break;
		case ValueType::INT64:   *(int64*)data *= value.GetInt64(); break;
		case ValueType::REAL32:  *(real32*)data *= value.GetReal32(); break;
		case ValueType::REAL64:  *(real64*)data *= value.GetReal64(); break;
		case ValueType::CHAR:    *(char*)data *= value.GetChar(); break;
		case ValueType::BOOL:	 *(bool*)data *= value.GetBool(); break;
		}
	}

	inline void DivideEquals(const Value& value)
	{
		switch ((ValueType)type)
		{
		case ValueType::UINT8:   *(uint8*)data /= value.GetUInt8(); break;
		case ValueType::UINT16:  *(uint16*)data /= value.GetUInt16(); break;
		case ValueType::UINT32:  *(uint32*)data /= value.GetUInt32(); break;
		case ValueType::UINT64:  *(uint64*)data /= value.GetUInt64(); break;
		case ValueType::INT8:    *(int8*)data /= value.GetInt8(); break;
		case ValueType::INT16:   *(int16*)data /= value.GetInt16(); break;
		case ValueType::INT32:   *(int32*)data /= value.GetInt32(); break;
		case ValueType::INT64:   *(int64*)data /= value.GetInt64(); break;
		case ValueType::REAL32:  *(real32*)data /= value.GetReal32(); break;
		case ValueType::REAL64:  *(real64*)data /= value.GetReal64(); break;
		case ValueType::CHAR:    *(char*)data /= value.GetChar(); break;
		case ValueType::BOOL:	 *(bool*)data /= value.GetBool(); break;
		}
	}

	Value Clone(Program* program, Allocator* allocator) const;
	Value CastTo(Program* program, uint16 newType, uint8 pointerLevel, Allocator* allocator) const;

	inline void Assign(const Value& value, uint64 typeSize)
	{
		void* target = isReference ? *(void**)data : data;
		Value source = value.Actual();

		if (!isReference && IsPointer() && value.IsPointer())
		{
			if (pointerLevel != value.pointerLevel || type != value.type) return;
			memcpy(target, source.data, sizeof(void*));
			return;
		}

		switch ((ValueType)type)
		{
		case ValueType::UINT8:   *(uint8*)target = source.GetUInt8(); break;
		case ValueType::UINT16:  *(uint16*)target = source.GetUInt16(); break;
		case ValueType::UINT32:  *(uint32*)target = source.GetUInt32(); break;
		case ValueType::UINT64:  *(uint64*)target = source.GetUInt64(); break;
		case ValueType::INT8:    *(int8*)target = source.GetInt8(); break;
		case ValueType::INT16:   *(int16*)target = source.GetInt16(); break;
		case ValueType::INT32:   *(int32*)target = source.GetInt32(); break;
		case ValueType::INT64:   *(int64*)target = source.GetInt64(); break;
		case ValueType::REAL32:  *(real32*)target = source.GetReal32(); break;
		case ValueType::REAL64:  *(real64*)target = source.GetReal64(); break;
		case ValueType::BOOL:    *(bool*)target = source.GetBool(); break;
		case ValueType::CHAR:    *(char*)target = source.GetChar(); break;
		default:
			if (type != value.type) return;
			memcpy(target, source.data, typeSize);
			break;
		}
	}

	inline void AssignOffset(Value& value, uint16 type, uint8 pointerLevel, uint64 typeSize, uint64 offset)
	{
		if (pointerLevel != value.pointerLevel) return;

		Value v = *this;
		if (IsPointer()) 
			v = Dereference();

		switch ((ValueType)type)
		{
		case ValueType::UINT8:   *(uint8*)((uint8*)data + offset) = value.GetUInt8(); break;
		case ValueType::UINT16:  *(uint16*)((uint8*)data + offset) = value.GetUInt16(); break;
		case ValueType::UINT32:  *(uint32*)((uint8*)data + offset) = value.GetUInt32(); break;
		case ValueType::UINT64:  *(uint64*)((uint8*)data + offset) = value.GetUInt64(); break;
		case ValueType::INT8:    *(int8*)((uint8*)data + offset) = value.GetInt8(); break;
		case ValueType::INT16:   *(int16*)((uint8*)data + offset) = value.GetInt16(); break;
		case ValueType::INT32:   *(int32*)((uint8*)data + offset) = value.GetInt32(); break;
		case ValueType::INT64:   *(int64*)((uint8*)data + offset) = value.GetInt64(); break;
		case ValueType::REAL32:  *(real32*)((uint8*)data + offset) = value.GetReal32(); break;
		case ValueType::REAL64:  *(real64*)((uint8*)data + offset) = value.GetReal64(); break;
		case ValueType::BOOL:    *(bool*)((uint8*)data + offset) = value.GetBool(); break;
		case ValueType::CHAR:    *(char*)((uint8*)data + offset) = value.GetChar(); break;
		default: {
			if (pointerLevel > 0)
			{
				*(void**)((uint8*)v.data + offset) = value.data;
			}
			else
			{
				memcpy((uint8*)v.data + offset, value.data, typeSize);
			}
		} break;
		}
	}

	inline Value Dereference() const
	{
		Value value;
		value.type = type;
		value.pointerLevel = pointerLevel - 1;
		value.isArray = isArray;
		value.data = *(void**)data;

		return value;
	}

	inline Value Actual() const
	{
		if (!isReference) return *this;

		Value value;
		value.type = type;
		value.pointerLevel = pointerLevel;
		value.isReference = false;
		value.isArray = false;
		value.data = *(void**)data;

		return value;
	}

	inline uint32 Calculate1DArrayIndex(const uint32* indices)
	{
		ArrayHeader* header = (ArrayHeader*)((uint8*)data - sizeof(ArrayHeader));
		
		uint32 index = 0;
		uint32 stride = 1;

		for (int32 i = header->numDimensions - 1; i >= 0; i--)
		{
			index += indices[i] * stride;
			stride *= header->dimensions[i];
		}

		return index;
	}

	inline static bool IsPrimitiveType(uint16 type) { return (type > (uint16)ValueType::FIRST_TYPE) && (type < (uint16)ValueType::LAST_TYPE); }
	inline static bool IsRealType(uint16 type) { return (type == (uint16)ValueType::REAL32) || (type == (uint16)ValueType::REAL64); }
	inline static bool IsIntegerType(uint16 type)
	{
		switch ((ValueType)type)
		{
		case ValueType::UINT8:
		case ValueType::UINT16:
		case ValueType::UINT32:
		case ValueType::UINT64:
		case ValueType::INT8:
		case ValueType::INT16:
		case ValueType::INT32:
		case ValueType::INT64:
			return true;
		default:
			return false;
		}
	}

	inline static int32 GetTypeRank(uint16 type) {
		switch ((ValueType)type) {
		case ValueType::BOOL:   return 0;
		case ValueType::CHAR:   return 1;

		case ValueType::INT8:   return 2;
		case ValueType::UINT8:  return 3;
		case ValueType::INT16:  return 4;
		case ValueType::UINT16: return 5;
		case ValueType::INT32:  return 6;
		case ValueType::UINT32: return 7;
		case ValueType::INT64:  return 8;
		case ValueType::UINT64: return 9;

		case ValueType::REAL32: return 10;
		case ValueType::REAL64: return 11;
		default: return -1;
		}
	}

	inline static uint16 PromoteType(uint16 a, uint16 b)
	{
		if (a == b) return a; // same type, preserve it

		int rankA = GetTypeRank(a);
		int rankB = GetTypeRank(b);

		return (rankA > rankB) ? a : b;
	}

	inline static Value MakeUInt8(uint8 v, Allocator* allocator)
	{
		Value value;
		value.type = (uint16)ValueType::UINT8;
		value.data = allocator->Alloc(sizeof(uint8));
		*(uint8*)value.data = v;
		value.pointerLevel = 0;
		value.isArray = false;
		value.isReference = false;
		return value;
	}

	inline static Value MakeUInt16(uint16 v, Allocator* allocator)
	{
		Value value;
		value.type = (uint16)ValueType::UINT16;
		value.data = allocator->Alloc(sizeof(uint16));
		*(uint16*)value.data = v;
		value.pointerLevel = 0;
		value.isArray = false;
		value.isReference = false;
		return value;
	}

	inline static Value MakeUInt32(uint32 v, Allocator* allocator)
	{
		Value value;
		value.type = (uint16)ValueType::UINT32;
		value.data = allocator->Alloc(sizeof(uint32));
		*(uint32*)value.data = v;
		value.pointerLevel = 0;
		value.isArray = false;
		value.isReference = false;
		return value;
	}

	inline static Value MakeUInt64(uint64 v, Allocator* allocator)
	{
		Value value;
		value.type = (uint16)ValueType::UINT64;
		value.data = allocator->Alloc(sizeof(uint64));
		*(uint64*)value.data = v;
		value.pointerLevel = 0;
		value.isArray = false;
		value.isReference = false;
		return value;
	}

	inline static Value MakeInt8(int8 v, Allocator* allocator)
	{
		Value value;
		value.type = (uint16)ValueType::INT8;
		value.data = allocator->Alloc(sizeof(int8));
		*(int8*)value.data = v;
		value.pointerLevel = 0;
		value.isArray = false;
		value.isReference = false;
		return value;
	}

	inline static Value MakeInt16(int16 v, Allocator* allocator)
	{
		Value value;
		value.type = (uint16)ValueType::INT16;
		value.data = allocator->Alloc(sizeof(int16));
		*(int16*)value.data = v;
		value.pointerLevel = 0;
		value.isArray = false;
		value.isReference = false;
		return value;
	}

	inline static Value MakeInt32(int32 v, Allocator* allocator)
	{
		Value value;
		value.type = (uint16)ValueType::INT32;
		value.data = allocator->Alloc(sizeof(int32));
		*(int32*)value.data = v;
		value.pointerLevel = 0;
		value.isArray = false;
		value.isReference = false;
		return value;
	}

	inline static Value MakeInt64(int64 v, Allocator* allocator)
	{
		Value value;
		value.type = (uint16)ValueType::INT64;
		value.data = allocator->Alloc(sizeof(int64));
		*(int64*)value.data = v;
		value.pointerLevel = 0;
		value.isArray = false;
		value.isReference = false;
		return value;
	}

	inline static Value MakeReal32(real32 v, Allocator* allocator)
	{
		Value value;
		value.type = (uint16)ValueType::REAL32;
		value.data = allocator->Alloc(sizeof(real32));
		*(real32*)value.data = v;
		value.pointerLevel = 0;
		value.isArray = false;
		value.isReference = false;
		return value;
	}

	inline static Value MakeReal64(real64 v, Allocator* allocator)
	{
		Value value;
		value.type = (uint16)ValueType::REAL64;
		value.data = allocator->Alloc(sizeof(real64));
		*(real64*)value.data = v;
		value.pointerLevel = 0;
		value.isReference = false;
		return value;
	}

	inline static Value MakeBool(bool v, Allocator* allocator)
	{
		Value value;
		value.type = (uint16)ValueType::BOOL;
		value.data = allocator->Alloc(sizeof(bool));
		*(bool*)value.data = v;
		value.pointerLevel = 0;
		value.isArray = false;
		value.isReference = false;
		return value;
	}

	inline static Value MakeChar(char v, Allocator* allocator)
	{
		Value value;
		value.type = (uint16)ValueType::CHAR;
		value.data = allocator->Alloc(sizeof(char));
		*(char*)value.data = v;
		value.pointerLevel = 0;
		value.isArray = false;
		value.isReference = false;
		return value;
	}

	inline static Value MakeCStr(std::string v, Allocator* allocator)
	{
		Value value;
		value.type = (uint16)ValueType::CHAR;
		value.data = allocator->Alloc(v.length() + 1);
		value.pointerLevel = 1;
		value.isArray = false;
		value.isReference = false;

		strcpy((char*)value.data, v.c_str());
		return value;
	}

	inline static Value MakePointer(uint16 type, uint8 pointerLevel, void* data, Allocator* allocator)
	{
		Value value;
		value.type = type;
		value.pointerLevel = pointerLevel;
		value.isArray = false;
		value.data = allocator->Alloc(sizeof(void*));
		*(void**)value.data = data;
		value.isReference = false;

		return value;
	}

	inline static Value MakeReference(const Value& v, Allocator* allocator)
	{
		Value value;
		value.type = v.type;
		value.pointerLevel = v.pointerLevel;
		value.isArray = false;
		value.isReference = true;
		value.data = allocator->Alloc(sizeof(void*));

		void* targetData = v.data;
		if (v.isReference)
			targetData = *(void**)v.data;

		*(void**)value.data = targetData;
		return value;
	}

	inline static Value MakeNULL(uint16 type = INVALID_ID, uint8 pointerLevel = 0)
	{
		Value value;
		value.type = type;
		value.pointerLevel = pointerLevel;
		value.isArray = false;
		value.data = nullptr;
		return value;
	}

	static Value MakeArray(Program* program, uint16 type, uint8 elementPointerLevel, uint32* dimension, uint32 numDimensions, Allocator* allocator);
	static Value MakeObject(Program* program, uint16 type, Allocator* allocator);
};