///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2012-2019 DreamWorks Animation LLC
//
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
//
// Redistributions of source code must retain the above copyright
// and license notice and the following restrictions and disclaimer.
//
// *     Neither the name of DreamWorks Animation nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// IN NO EVENT SHALL THE COPYRIGHT HOLDERS' AND CONTRIBUTORS' AGGREGATE
// LIABILITY FOR ALL CLAIMS REGARDLESS OF THEIR BASIS EXCEED US$250.00.
//
///////////////////////////////////////////////////////////////////////////

#ifndef OPENVDB_TYPES_HAS_BEEN_INCLUDED
#define OPENVDB_TYPES_HAS_BEEN_INCLUDED

#include "version.h"
#include "Platform.h"
#include <OpenEXR/half.h>
#include <openvdb/math/Math.h>
#include <openvdb/math/BBox.h>
#include <openvdb/math/Quat.h>
#include <openvdb/math/Vec2.h>
#include <openvdb/math/Vec3.h>
#include <openvdb/math/Vec4.h>
#include <openvdb/math/Mat3.h>
#include <openvdb/math/Mat4.h>
#include <openvdb/math/Coord.h>
#include <assert.h>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <type_traits>
#if OPENVDB_ABI_VERSION_NUMBER <= 3
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#ifdef OPENVDB_USE_TBB
#include <tbb/blocked_range.h>
#include <tbb/blocked_range2d.h>
#include <tbb/blocked_range3d.h>
#include <tbb/combinable.h>
#include <tbb/enumerable_thread_specific.h>
#endif


namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

// One-dimensional scalar types
using Index32 = uint32_t;
using Index64 = uint64_t;
using Index   = Index32;
using Int16   = int16_t;
using Int32   = int32_t;
using Int64   = int64_t;
using Int     = Int32;
using Byte    = unsigned char;
using Real    = double;

// Two-dimensional vector types
using Vec2R = math::Vec2<Real>;
using Vec2I = math::Vec2<Index32>;
using Vec2f = math::Vec2<float>;
using Vec2H = math::Vec2<half>;
using math::Vec2i;
using math::Vec2s;
using math::Vec2d;

// Three-dimensional vector types
using Vec3R = math::Vec3<Real>;
using Vec3I = math::Vec3<Index32>;
using Vec3f = math::Vec3<float>;
using Vec3H = math::Vec3<half>;
using Vec3U8 = math::Vec3<uint8_t>;
using Vec3U16 = math::Vec3<uint16_t>;
using math::Vec3i;
using math::Vec3s;
using math::Vec3d;

using math::Coord;
using math::CoordBBox;
using BBoxd = math::BBox<Vec3d>;

// Four-dimensional vector types
using Vec4R = math::Vec4<Real>;
using Vec4I = math::Vec4<Index32>;
using Vec4f = math::Vec4<float>;
using Vec4H = math::Vec4<half>;
using math::Vec4i;
using math::Vec4s;
using math::Vec4d;

// Three-dimensional matrix types
using Mat3R = math::Mat3<Real>;
using math::Mat3s;
using math::Mat3d;

// Four-dimensional matrix types
using Mat4R = math::Mat4<Real>;
using math::Mat4s;
using math::Mat4d;

// Quaternions
using QuatR = math::Quat<Real>;
using math::Quats;
using math::Quatd;

// Dummy type for a voxel with a binary mask value, e.g. the active state
class ValueMask {};


#if OPENVDB_ABI_VERSION_NUMBER <= 3

// Use Boost shared pointers in OpenVDB 3 ABI compatibility mode.
template<typename T> using SharedPtr = boost::shared_ptr<T>;
template<typename T> using WeakPtr = boost::weak_ptr<T>;

template<typename T, typename U> inline SharedPtr<T>
ConstPtrCast(const SharedPtr<U>& ptr) { return boost::const_pointer_cast<T, U>(ptr); }

template<typename T, typename U> inline SharedPtr<T>
DynamicPtrCast(const SharedPtr<U>& ptr) { return boost::dynamic_pointer_cast<T, U>(ptr); }

template<typename T, typename U> inline SharedPtr<T>
StaticPtrCast(const SharedPtr<U>& ptr) { return boost::static_pointer_cast<T, U>(ptr); }

#else // if OPENVDB_ABI_VERSION_NUMBER > 3

// Use STL shared pointers from OpenVDB 4 on.
template<typename T> using SharedPtr = std::shared_ptr<T>;
template<typename T> using WeakPtr = std::weak_ptr<T>;

/// @brief Return a new shared pointer that points to the same object
/// as the given pointer but with possibly different <TT>const</TT>-ness.
/// @par Example:
/// @code
/// FloatGrid::ConstPtr grid = ...;
/// FloatGrid::Ptr nonConstGrid = ConstPtrCast<FloatGrid>(grid);
/// FloatGrid::ConstPtr constGrid = ConstPtrCast<const FloatGrid>(nonConstGrid);
/// @endcode
template<typename T, typename U> inline SharedPtr<T>
ConstPtrCast(const SharedPtr<U>& ptr) { return std::const_pointer_cast<T, U>(ptr); }

/// @brief Return a new shared pointer that is either null or points to
/// the same object as the given pointer after a @c dynamic_cast.
/// @par Example:
/// @code
/// GridBase::ConstPtr grid = ...;
/// FloatGrid::ConstPtr floatGrid = DynamicPtrCast<const FloatGrid>(grid);
/// @endcode
template<typename T, typename U> inline SharedPtr<T>
DynamicPtrCast(const SharedPtr<U>& ptr) { return std::dynamic_pointer_cast<T, U>(ptr); }

/// @brief Return a new shared pointer that points to the same object
/// as the given pointer after a @c static_cast.
/// @par Example:
/// @code
/// FloatGrid::Ptr floatGrid = ...;
/// GridBase::Ptr grid = StaticPtrCast<GridBase>(floatGrid);
/// @endcode
template<typename T, typename U> inline SharedPtr<T>
StaticPtrCast(const SharedPtr<U>& ptr) { return std::static_pointer_cast<T, U>(ptr); }

#endif


////////////////////////////////////////


/// @brief  Integer wrapper, required to distinguish PointIndexGrid and
///         PointDataGrid from Int32Grid and Int64Grid
/// @note   @c Kind is a dummy parameter used to create distinct types.
template<typename IntType_, Index Kind>
struct PointIndex
{
    static_assert(std::is_integral<IntType_>::value, "PointIndex requires an integer value type");

    using IntType = IntType_;

    PointIndex(IntType i = IntType(0)): mIndex(i) {}

    /// Explicit type conversion constructor
    template<typename T> explicit PointIndex(T i): mIndex(static_cast<IntType>(i)) {}

    operator IntType() const { return mIndex; }

    /// Needed to support the <tt>(zeroVal<PointIndex>() + val)</tt> idiom.
    template<typename T>
    PointIndex operator+(T x) { return PointIndex(mIndex + IntType(x)); }

private:
    IntType mIndex;
};


using PointIndex32 = PointIndex<Index32, 0>;
using PointIndex64 = PointIndex<Index64, 0>;

using PointDataIndex32 = PointIndex<Index32, 1>;
using PointDataIndex64 = PointIndex<Index64, 1>;


////////////////////////////////////////


template <typename T>
constexpr size_t sizeof_array(const T& iarray)
{
    return (sizeof(iarray) / sizeof(iarray[0]));
}


////////////////////////////////////////


template <int NBits>
struct ShortestFittingInt
{
	static_assert(NBits > 0, "Negative or Zero NBits");

	enum { NBytes = ((NBits - 1) / 8) + 1 };

	static_assert(NBytes > 0, "Negative or Zero NBytes");
	static_assert(NBytes <= sizeof(int64_t), "NBytes > 8");

	using Type = typename std::conditional<
		(NBytes <= sizeof(int8_t)),
		int8_t,
		typename std::conditional<
			(NBytes <= sizeof(int16_t)),
			int16_t,
			typename std::conditional<
				(NBytes <= sizeof(int32_t)),
				int32_t,
				int64_t>::type>::type>::type;
};

template <int NBits>
using ShortestFittingIntT = typename ShortestFittingInt<NBits>::Type;


////////////////////////////////////////


/// @brief Helper metafunction used to determine if the first template
/// parameter is a specialization of the class template given in the second
/// template parameter
template <typename T, template <typename...> class Template>
struct IsSpecializationOf : std::false_type {};

template <typename... Args, template <typename...> class Template>
struct IsSpecializationOf<Template<Args...>, Template> : std::true_type {};


////////////////////////////////////////


struct NullMutex
{
	void lock() {}
	void unlock() noexcept {}
	bool try_lock() { return true; }
};

    
////////////////////////////////////////


#ifdef OPENVDB_USE_TBB
template <typename T>
using BlockedRange = typename tbb::blocked_range<T>;
template<typename T, typename U = T>
using BlockedRange2D = typename tbb::blocked_range2d<T, U>;
template<typename T, typename U = T, typename V = U>
using BlockedRange3D = typename tbb::blocked_range3d<T, U, V>;
#else
template<typename T>
class BlockedRange
{
public:
	
	typedef T const_iterator;
	typedef std::size_t size_type;

	BlockedRange() : mEnd(), mBegin(), mGrainSize() {}
	BlockedRange(T begin_, T end_, size_type grainsize_ = 1) :
		mEnd(end_), mBegin(begin_), mGrainSize(grainsize_) { }

	const_iterator begin() const { return mBegin; }
	const_iterator end() const { return mEnd; }

	size_type size() const { return size_type(mEnd - mBegin); }

	size_type grainsize() const { return mGrainSize; }
	bool empty() const { return !(mBegin < mEnd); }
	bool is_divisible() const { return mGrainSize < size(); }

private:
	T mEnd;
	T mBegin;
	size_type mGrainSize;
};

template<typename RowValue, typename ColValue = RowValue>
class BlockedRange2D
{
public:

	typedef BlockedRange<RowValue>  RowRangeT;
	typedef BlockedRange<ColValue>  ColRangeT;

	BlockedRange2D(
		RowValue rowBegin, RowValue rowEnd,
		ColValue col_begin, ColValue col_end)
		: mRows(rowBegin, rowEnd)
		, mCols(col_begin, col_end) { }

	BlockedRange2D(
		RowValue row_begin, RowValue row_end, typename RowRangeT::size_type rowGrainSize,
		ColValue colBegin, ColValue colEnd, typename ColRangeT::size_type colGrainSize)
		: mRows(row_begin, row_end, rowGrainSize)
		, mCols(colBegin, colEnd, colGrainSize) { }

	bool empty() const { return mRows.empty() || mCols.empty(); }
	bool is_divisible() const { return  mRows.is_divisible() || mCols.is_divisible(); }

	const RowRangeT& rows() const { return mRows; }
	const ColRangeT& cols() const { return mCols; }

private:
	RowRangeT  mRows;
	ColRangeT  mCols;
};

template<typename PageValue, typename RowValue = PageValue, typename ColValue = RowValue>
class BlockedRange3D
{
public:

	typedef BlockedRange<PageValue> PageRangeT;
	typedef BlockedRange<RowValue>  RowRangeT;
	typedef BlockedRange<ColValue>  ColRangeT;

	BlockedRange3D(
		PageValue pageBegin, PageValue pageEnd,
		RowValue rowBegin, RowValue rowEnd,
		ColValue col_begin, ColValue col_end)
		: mPages(pageBegin, pageEnd)
		, mRows(rowBegin, rowEnd)
		, mCols(col_begin, col_end) { }

	BlockedRange3D(
		PageValue page_begin, PageValue page_end, typename PageRangeT::size_type pageGrainSize,
		RowValue row_begin, RowValue row_end, typename RowRangeT::size_type rowGrainSize,
		ColValue colBegin, ColValue colEnd, typename ColRangeT::size_type colGrainSize)
		: mPages(page_begin, page_end, pageGrainSize)
		, mRows(row_begin, row_end, rowGrainSize)
		, mCols(colBegin, colEnd, colGrainSize) { }

	bool empty() const { return mPages.empty() || mRows.empty() || mCols.empty(); }
	bool is_divisible() const { return  mPages.is_divisible() || mRows.is_divisible() || mCols.is_divisible(); }

	const PageRangeT& pages() const { return mPages; }
	const RowRangeT& rows() const { return mRows; }
	const ColRangeT& cols() const { return mCols; }

private:
	PageRangeT mPages;
	RowRangeT  mRows;
	ColRangeT  mCols;
};
#endif


////////////////////////////////////////
#ifdef OPENVDB_USE_TBB
    template <typename T>
    using EnumerableThreadSpecific = typename tbb::enumerable_thread_specific<T>;
    template <typename T>
    using Combinable = typename tbb::combinable<T>;
#else
    template <typename T>
    class Combinable
    {
    public:
        using Type = Combinable<T>;
        using ContainerType = typename std::list<T>;
        using MutexType = std::mutex;
        
        Combinable() { createLocal(); }
        explicit Combinable(const T& val) { createLocalCopy(val); }
        virtual ~Combinable() { clear(); }
        
        void clear()
        {
            mLocal = nullptr;
            
            std::lock_guard<std::mutex> lock(sAllLocalsMutex);
            
            sAllLocals.clear();
        }
        
        T& local()
        {
            bool exists;
            return local(exists);
        }
        
        T& local(bool& exists)
        {
            if (mLocal)
            {
                exists = true;
            }
            else
            {
                createLocal();
                exists = false;
            }
            
            return *mLocal;
        }
        
        size_t size() const
        {
            std::lock_guard<MutexType> lock(sAllLocalsMutex);
            
            return sAllLocals.size();
        }
        
        bool empty() const
        {
            std::lock_guard<MutexType> lock(sAllLocalsMutex);
            
            return sAllLocals.empty();
        }
        
        template<typename Function>
        T combine(Function binaryCombineFcn) const
        {
            std::lock_guard<MutexType> lock(sAllLocalsMutex);
            
            if (sAllLocals.empty())
                return T();
            
            T result;
            for (auto& local : sAllLocals)
                result = binaryCombineFcn(result, local);
            
            return result;
        }
        
        template<typename Function>
        void combine_each(Function unaryCombineFunction) const
        {
            std::lock_guard<MutexType> lock(sAllLocalsMutex);
            
            for (auto& local : sAllLocals)
                unaryCombineFunction(local);
        }
        
    protected:
        
        void createLocal()
        {
            std::lock_guard<MutexType> lock(sAllLocalsMutex);
            
            sAllLocals.emplace_back();
            mLocal = &sAllLocals.back();
        }
        
        void createLocalCopy(const T& val)
        {
            std::lock_guard<MutexType> lock(sAllLocalsMutex);
            
            sAllLocals.push_back(val);
            mLocal = &sAllLocals.back();
        }
        
        thread_local static T* mLocal;
        static MutexType sAllLocalsMutex;
        static ContainerType sAllLocals;
    };
    
    template<typename T>
    std::mutex Combinable<T>::sAllLocalsMutex;
    
    template<typename T>
    std::list<T> Combinable<T>::sAllLocals;
    
    template<typename T>
    thread_local T* Combinable<T>::mLocal(nullptr);
    
    template <typename T>
    class EnumerableThreadSpecific : public Combinable<T>
    {
    public:
        
        using iterator = typename Combinable<T>::ContainerType::iterator;
        using const_iterator = typename Combinable<T>::ContainerType::const_iterator;
        using MutexType = typename Combinable<T>::MutexType;
        
        EnumerableThreadSpecific() = default;
        explicit EnumerableThreadSpecific(const T& val) { this->createLocalCopy(val); }
        ~EnumerableThreadSpecific() = default;
        
        iterator begin()
        {
            std::lock_guard<MutexType> lock(this->sAllLocalsMutex);
            
            return this->sAllLocals.begin();
        }
        iterator end()
        {
            std::lock_guard<MutexType> lock(this->sAllLocalsMutex);
            
            return this->sAllLocals.end();
        }
        const_iterator cbegin() const
        {
            std::lock_guard<MutexType> lock(this->sAllLocalsMutex);
            
            return this->sAllLocals.cbegin();
        }
        const_iterator cend() const
        {
            std::lock_guard<MutexType> lock(this->sAllLocalsMutex);
            
            return this->sAllLocals.cend();
        }
        
        BlockedRange<size_t> range(size_t grainsize = 1)
        {
            std::lock_guard<MutexType> lock(this->sAllLocalsMutex);
            
            return BlockedRange<size_t>(this->sAllLocals.begin(), this->sAllLocals.end(), grainsize);
        }
        const BlockedRange<size_t> range(size_t grainsize = 1) const
        {
            std::lock_guard<MutexType> lock(this->sAllLocalsMutex);
            
            return BlockedRange<size_t>(this->sAllLocals.cbegin(), this->sAllLocals.cend(), grainsize);
        }
    };
#endif
    
    
////////////////////////////////////////


#ifdef OPENVDB_USE_TBB
using SimplePartitioner = typename tbb::simple_partitioner;
#else
using SimplePartitioner = void;
#endif


////////////////////////////////////////


template<typename T, bool = IsSpecializationOf<T, math::Vec2>::value ||
                            IsSpecializationOf<T, math::Vec3>::value ||
                            IsSpecializationOf<T, math::Vec4>::value>
struct VecTraits
{
    static const bool IsVec = true;
    static const int Size = T::size;
    using ElementType = typename T::ValueType;
};

template<typename T>
struct VecTraits<T, false>
{
    static const bool IsVec = false;
    static const int Size = 1;
    using ElementType = T;
};

template<typename T, bool = IsSpecializationOf<T, math::Quat>::value>
struct QuatTraits
{
    static const bool IsQuat = true;
    static const int Size = T::size;
    using ElementType = typename T::ValueType;
};

template<typename T>
struct QuatTraits<T, false>
{
    static const bool IsQuat = false;
    static const int Size = 1;
    using ElementType = T;
};

template<typename T, bool = IsSpecializationOf<T, math::Mat3>::value ||
                            IsSpecializationOf<T, math::Mat4>::value>
struct MatTraits
{
    static const bool IsMat = true;
    static const int Size = T::size;
    using ElementType = typename T::ValueType;
};

template<typename T>
struct MatTraits<T, false>
{
    static const bool IsMat = false;
    static const int Size = 1;
    using ElementType = T;
};

template<typename T, bool = VecTraits<T>::IsVec ||
                            QuatTraits<T>::IsQuat ||
                            MatTraits<T>::IsMat>
struct ValueTraits
{
    static const bool IsVec = VecTraits<T>::IsVec;
    static const bool IsQuat = QuatTraits<T>::IsQuat;
    static const bool IsMat = MatTraits<T>::IsMat;
    static const bool IsScalar = false;
    static const int Size = T::size;
    static const int Elements = IsMat ? Size*Size : Size;
    using ElementType = typename T::ValueType;
};

template<typename T>
struct ValueTraits<T, false>
{
    static const bool IsVec = false;
    static const bool IsQuat = false;
    static const bool IsMat = false;
    static const bool IsScalar = true;
    static const int Size = 1;
    static const int Elements = 1;
    using ElementType = T;
};


////////////////////////////////////////


/// @brief CanConvertType<FromType, ToType>::value is @c true if a value
/// of type @a ToType can be constructed from a value of type @a FromType.
template<typename FromType, typename ToType>
struct CanConvertType { enum { value = std::is_constructible<ToType, FromType>::value }; };

// Specializations for vector types, which can be constructed from values
// of their own ValueTypes (or values that can be converted to their ValueTypes),
// but only explicitly
template<typename T> struct CanConvertType<T, math::Vec2<T> > { enum { value = true }; };
template<typename T> struct CanConvertType<T, math::Vec3<T> > { enum { value = true }; };
template<typename T> struct CanConvertType<T, math::Vec4<T> > { enum { value = true }; };
template<typename T> struct CanConvertType<math::Vec2<T>, math::Vec2<T> > { enum {value = true}; };
template<typename T> struct CanConvertType<math::Vec3<T>, math::Vec3<T> > { enum {value = true}; };
template<typename T> struct CanConvertType<math::Vec4<T>, math::Vec4<T> > { enum {value = true}; };
template<typename T0, typename T1>
struct CanConvertType<T0, math::Vec2<T1> > { enum { value = CanConvertType<T0, T1>::value }; };
template<typename T0, typename T1>
struct CanConvertType<T0, math::Vec3<T1> > { enum { value = CanConvertType<T0, T1>::value }; };
template<typename T0, typename T1>
struct CanConvertType<T0, math::Vec4<T1> > { enum { value = CanConvertType<T0, T1>::value }; };
template<> struct CanConvertType<PointIndex32, PointDataIndex32> { enum {value = true}; };
template<> struct CanConvertType<PointDataIndex32, PointIndex32> { enum {value = true}; };
template<typename T>
struct CanConvertType<T, ValueMask> { enum {value = CanConvertType<T, bool>::value}; };
template<typename T>
struct CanConvertType<ValueMask, T> { enum {value = CanConvertType<bool, T>::value}; };


////////////////////////////////////////


// Add new items to the *end* of this list, and update NUM_GRID_CLASSES.
enum GridClass {
    GRID_UNKNOWN = 0,
    GRID_LEVEL_SET,
    GRID_FOG_VOLUME,
    GRID_STAGGERED
};
enum { NUM_GRID_CLASSES = GRID_STAGGERED + 1 };

static const Real LEVEL_SET_HALF_WIDTH = 3;

/// The type of a vector determines how transforms are applied to it:
/// <dl>
/// <dt><b>Invariant</b>
/// <dd>Does not transform (e.g., tuple, uvw, color)
///
/// <dt><b>Covariant</b>
/// <dd>Apply inverse-transpose transformation: @e w = 0, ignores translation
///     (e.g., gradient/normal)
///
/// <dt><b>Covariant Normalize</b>
/// <dd>Apply inverse-transpose transformation: @e w = 0, ignores translation,
///     vectors are renormalized (e.g., unit normal)
///
/// <dt><b>Contravariant Relative</b>
/// <dd>Apply "regular" transformation: @e w = 0, ignores translation
///     (e.g., displacement, velocity, acceleration)
///
/// <dt><b>Contravariant Absolute</b>
/// <dd>Apply "regular" transformation: @e w = 1, vector translates (e.g., position)
/// </dl>
enum VecType {
    VEC_INVARIANT = 0,
    VEC_COVARIANT,
    VEC_COVARIANT_NORMALIZE,
    VEC_CONTRAVARIANT_RELATIVE,
    VEC_CONTRAVARIANT_ABSOLUTE
};
enum { NUM_VEC_TYPES = VEC_CONTRAVARIANT_ABSOLUTE + 1 };


/// Specify how grids should be merged during certain (typically multithreaded) operations.
/// <dl>
/// <dt><b>MERGE_ACTIVE_STATES</b>
/// <dd>The output grid is active wherever any of the input grids is active.
///
/// <dt><b>MERGE_NODES</b>
/// <dd>The output grid's tree has a node wherever any of the input grids' trees
///     has a node, regardless of any active states.
///
/// <dt><b>MERGE_ACTIVE_STATES_AND_NODES</b>
/// <dd>The output grid is active wherever any of the input grids is active,
///     and its tree has a node wherever any of the input grids' trees has a node.
/// </dl>
enum MergePolicy {
    MERGE_ACTIVE_STATES = 0,
    MERGE_NODES,
    MERGE_ACTIVE_STATES_AND_NODES
};


////////////////////////////////////////


template<typename T> const char* typeNameAsString()                 { return typeid(T).name(); }
template<> inline const char* typeNameAsString<bool>()              { return "bool"; }
template<> inline const char* typeNameAsString<ValueMask>()         { return "mask"; }
template<> inline const char* typeNameAsString<half>()              { return "half"; }
template<> inline const char* typeNameAsString<float>()             { return "float"; }
template<> inline const char* typeNameAsString<double>()            { return "double"; }
template<> inline const char* typeNameAsString<int8_t>()            { return "int8"; }
template<> inline const char* typeNameAsString<uint8_t>()           { return "uint8"; }
template<> inline const char* typeNameAsString<int16_t>()           { return "int16"; }
template<> inline const char* typeNameAsString<uint16_t>()          { return "uint16"; }
template<> inline const char* typeNameAsString<int32_t>()           { return "int32"; }
template<> inline const char* typeNameAsString<uint32_t>()          { return "uint32"; }
template<> inline const char* typeNameAsString<int64_t>()           { return "int64"; }
template<> inline const char* typeNameAsString<Vec2i>()             { return "vec2i"; }
template<> inline const char* typeNameAsString<Vec2s>()             { return "vec2s"; }
template<> inline const char* typeNameAsString<Vec2d>()             { return "vec2d"; }
template<> inline const char* typeNameAsString<Vec3U8>()            { return "vec3u8"; }
template<> inline const char* typeNameAsString<Vec3U16>()           { return "vec3u16"; }
template<> inline const char* typeNameAsString<Vec3i>()             { return "vec3i"; }
template<> inline const char* typeNameAsString<Vec3f>()             { return "vec3s"; }
template<> inline const char* typeNameAsString<Vec3d>()             { return "vec3d"; }
template<> inline const char* typeNameAsString<Vec4i>()             { return "vec4i"; }
template<> inline const char* typeNameAsString<Vec4f>()             { return "vec4s"; }
template<> inline const char* typeNameAsString<Vec4d>()             { return "vec4d"; }
template<> inline const char* typeNameAsString<std::string>()       { return "string"; }
template<> inline const char* typeNameAsString<Mat3s>()             { return "mat3s"; }
template<> inline const char* typeNameAsString<Mat3d>()             { return "mat3d"; }
template<> inline const char* typeNameAsString<Mat4s>()             { return "mat4s"; }
template<> inline const char* typeNameAsString<Mat4d>()             { return "mat4d"; }
template<> inline const char* typeNameAsString<math::Quats>()       { return "quats"; }
template<> inline const char* typeNameAsString<math::Quatd>()       { return "quatd"; }
template<> inline const char* typeNameAsString<PointIndex32>()      { return "ptidx32"; }
template<> inline const char* typeNameAsString<PointIndex64>()      { return "ptidx64"; }
template<> inline const char* typeNameAsString<PointDataIndex32>()  { return "ptdataidx32"; }
template<> inline const char* typeNameAsString<PointDataIndex64>()  { return "ptdataidx64"; }


////////////////////////////////////////


/// @brief This struct collects both input and output arguments to "grid combiner" functors
/// used with the tree::TypedGrid::combineExtended() and combine2Extended() methods.
/// AValueType and BValueType are the value types of the two grids being combined.
///
/// @see openvdb/tree/Tree.h for usage information.
///
/// Setter methods return references to this object, to facilitate the following usage:
/// @code
///     CombineArgs<float> args;
///     myCombineOp(args.setARef(aVal).setBRef(bVal).setAIsActive(true).setBIsActive(false));
/// @endcode
template<typename AValueType, typename BValueType = AValueType>
class CombineArgs
{
public:
    using AValueT = AValueType;
    using BValueT = BValueType;

    CombineArgs()
        : mAValPtr(nullptr)
        , mBValPtr(nullptr)
        , mResultValPtr(&mResultVal)
        , mAIsActive(false)
        , mBIsActive(false)
        , mResultIsActive(false)
    {
    }

    /// Use this constructor when the result value is stored externally.
    CombineArgs(const AValueType& a, const BValueType& b, AValueType& result,
                bool aOn = false, bool bOn = false)
        : mAValPtr(&a)
        , mBValPtr(&b)
        , mResultValPtr(&result)
        , mAIsActive(aOn)
        , mBIsActive(bOn)
    {
        this->updateResultActive();
    }

    /// Use this constructor when the result value should be stored in this struct.
    CombineArgs(const AValueType& a, const BValueType& b, bool aOn = false, bool bOn = false)
        : mAValPtr(&a)
        , mBValPtr(&b)
        , mResultValPtr(&mResultVal)
        , mAIsActive(aOn)
        , mBIsActive(bOn)
    {
        this->updateResultActive();
    }

    /// Get the A input value.
    const AValueType& a() const { return *mAValPtr; }
    /// Get the B input value.
    const BValueType& b() const { return *mBValPtr; }
    //@{
    /// Get the output value.
    const AValueType& result() const { return *mResultValPtr; }
    AValueType& result() { return *mResultValPtr; }
    //@}

    /// Set the output value.
    CombineArgs& setResult(const AValueType& val) { *mResultValPtr = val; return *this; }

    /// Redirect the A value to a new external source.
    CombineArgs& setARef(const AValueType& a) { mAValPtr = &a; return *this; }
    /// Redirect the B value to a new external source.
    CombineArgs& setBRef(const BValueType& b) { mBValPtr = &b; return *this; }
    /// Redirect the result value to a new external destination.
    CombineArgs& setResultRef(AValueType& val) { mResultValPtr = &val; return *this; }

    /// @return true if the A value is active
    bool aIsActive() const { return mAIsActive; }
    /// @return true if the B value is active
    bool bIsActive() const { return mBIsActive; }
    /// @return true if the output value is active
    bool resultIsActive() const { return mResultIsActive; }

    /// Set the active state of the A value.
    CombineArgs& setAIsActive(bool b) { mAIsActive = b; updateResultActive(); return *this; }
    /// Set the active state of the B value.
    CombineArgs& setBIsActive(bool b) { mBIsActive = b; updateResultActive(); return *this; }
    /// Set the active state of the output value.
    CombineArgs& setResultIsActive(bool b) { mResultIsActive = b; return *this; }

protected:
    /// By default, the result value is active if either of the input values is active,
    /// but this behavior can be overridden by calling setResultIsActive().
    void updateResultActive() { mResultIsActive = mAIsActive || mBIsActive; }

    const AValueType* mAValPtr;   // pointer to input value from A grid
    const BValueType* mBValPtr;   // pointer to input value from B grid
    AValueType mResultVal;        // computed output value (unused if stored externally)
    AValueType* mResultValPtr;    // pointer to either mResultVal or an external value
    bool mAIsActive, mBIsActive;  // active states of A and B values
    bool mResultIsActive;         // computed active state (default: A active || B active)
};


/// This struct adapts a "grid combiner" functor to swap the A and B grid values
/// (e.g., so that if the original functor computes a + 2 * b, the adapted functor
/// will compute b + 2 * a).
template<typename ValueType, typename CombineOp>
struct SwappedCombineOp
{
    SwappedCombineOp(CombineOp& _op): op(_op) {}

    void operator()(CombineArgs<ValueType>& args)
    {
        CombineArgs<ValueType> swappedArgs(args.b(), args.a(), args.result(),
            args.bIsActive(), args.aIsActive());
        op(swappedArgs);
    }

    CombineOp& op;
};


////////////////////////////////////////


#if OPENVDB_ABI_VERSION_NUMBER <= 3
/// In copy constructors, members stored as shared pointers can be handled
/// in several ways:
/// <dl>
/// <dt><b>CP_NEW</b>
/// <dd>Don't copy the member; default construct a new member object instead.
///
/// <dt><b>CP_SHARE</b>
/// <dd>Copy the shared pointer, so that the original and new objects share
///     the same member.
///
/// <dt><b>CP_COPY</b>
/// <dd>Create a deep copy of the member.
/// </dl>
enum CopyPolicy { CP_NEW, CP_SHARE, CP_COPY };
#endif


/// @brief Tag dispatch class that distinguishes shallow copy constructors
/// from deep copy constructors
class ShallowCopy {};
/// @brief Tag dispatch class that distinguishes topology copy constructors
/// from deep copy constructors
class TopologyCopy {};
/// @brief Tag dispatch class that distinguishes constructors during file input
class PartialCreate {};

} // namespace OPENVDB_VERSION_NAME
} // namespace openvdb


#if defined(__ICC)

// Use these defines to bracket a region of code that has safe static accesses.
// Keep the region as small as possible.
#define OPENVDB_START_THREADSAFE_STATIC_REFERENCE   __pragma(warning(disable:1710))
#define OPENVDB_FINISH_THREADSAFE_STATIC_REFERENCE  __pragma(warning(default:1710))
#define OPENVDB_START_THREADSAFE_STATIC_WRITE       __pragma(warning(disable:1711))
#define OPENVDB_FINISH_THREADSAFE_STATIC_WRITE      __pragma(warning(default:1711))
#define OPENVDB_START_THREADSAFE_STATIC_ADDRESS     __pragma(warning(disable:1712))
#define OPENVDB_FINISH_THREADSAFE_STATIC_ADDRESS    __pragma(warning(default:1712))

// Use these defines to bracket a region of code that has unsafe static accesses.
// Keep the region as small as possible.
#define OPENVDB_START_NON_THREADSAFE_STATIC_REFERENCE   __pragma(warning(disable:1710))
#define OPENVDB_FINISH_NON_THREADSAFE_STATIC_REFERENCE  __pragma(warning(default:1710))
#define OPENVDB_START_NON_THREADSAFE_STATIC_WRITE       __pragma(warning(disable:1711))
#define OPENVDB_FINISH_NON_THREADSAFE_STATIC_WRITE      __pragma(warning(default:1711))
#define OPENVDB_START_NON_THREADSAFE_STATIC_ADDRESS     __pragma(warning(disable:1712))
#define OPENVDB_FINISH_NON_THREADSAFE_STATIC_ADDRESS    __pragma(warning(default:1712))

// Simpler version for one-line cases
#define OPENVDB_THREADSAFE_STATIC_REFERENCE(CODE) \
    __pragma(warning(disable:1710)); CODE; __pragma(warning(default:1710))
#define OPENVDB_THREADSAFE_STATIC_WRITE(CODE) \
    __pragma(warning(disable:1711)); CODE; __pragma(warning(default:1711))
#define OPENVDB_THREADSAFE_STATIC_ADDRESS(CODE) \
    __pragma(warning(disable:1712)); CODE; __pragma(warning(default:1712))

#else // GCC does not support these compiler warnings

#define OPENVDB_START_THREADSAFE_STATIC_REFERENCE
#define OPENVDB_FINISH_THREADSAFE_STATIC_REFERENCE
#define OPENVDB_START_THREADSAFE_STATIC_WRITE
#define OPENVDB_FINISH_THREADSAFE_STATIC_WRITE
#define OPENVDB_START_THREADSAFE_STATIC_ADDRESS
#define OPENVDB_FINISH_THREADSAFE_STATIC_ADDRESS

#define OPENVDB_START_NON_THREADSAFE_STATIC_REFERENCE
#define OPENVDB_FINISH_NON_THREADSAFE_STATIC_REFERENCE
#define OPENVDB_START_NON_THREADSAFE_STATIC_WRITE
#define OPENVDB_FINISH_NON_THREADSAFE_STATIC_WRITE
#define OPENVDB_START_NON_THREADSAFE_STATIC_ADDRESS
#define OPENVDB_FINISH_NON_THREADSAFE_STATIC_ADDRESS

#define OPENVDB_THREADSAFE_STATIC_REFERENCE(CODE) CODE
#define OPENVDB_THREADSAFE_STATIC_WRITE(CODE) CODE
#define OPENVDB_THREADSAFE_STATIC_ADDRESS(CODE) CODE

#endif // defined(__ICC)

#endif // OPENVDB_TYPES_HAS_BEEN_INCLUDED

// Copyright (c) 2012-2019 DreamWorks Animation LLC
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
