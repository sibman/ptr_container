/**
 * Pointer Container Library
 * (C) Thorsten Ottosen 2003-2005
 * 
 * Permission to copy, use, modify,
 * sell and distribute this software is granted provided this
 * copyright notice appears in all copies. This software is provided
 * "as is" without express or implied warranty, and with no claim as
 * to its suitability for any purpose.
 *
 * See http://www.boost.org/libs/ptr_container for documentation.
 *
 */

#ifndef BOOST_PTR_CONTAINER_DETAIL_REVERSIBLE_PTR_CONTAINER_HPP
#define BOOST_PTR_CONTAINER_DETAIL_REVERSIBLE_PTR_CONTAINER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include <boost/ptr_container/detail/scoped_deleter.hpp>
#include <boost/ptr_container/detail/static_move_ptr.hpp>
#include <boost/ptr_container/exception.hpp>
#include <boost/ptr_container/clone_allocator.hpp>
#include <boost/ptr_container/nullable.hpp>
#include <boost/range/functions.hpp>
#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/iterator/indirect_iterator.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/operators.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <algorithm>
#include <exception>
#include <memory>

namespace boost
{
    
namespace ptr_container_detail
{

    template< class CloneAllocator >
    struct clone_deleter
    {
        template< class T >
        void operator()( const T* p ) const
        {
            CloneAllocator::deallocate_clone( p );
        }
    };

    template< class T >
    struct is_pointer_or_integral
    {
        BOOST_STATIC_CONSTANT(bool, value = is_pointer<T>::value || is_integral<T>::value );
    };
    
    template
    < 
        class Config, 
        class CloneAllocator
    >
    class reversible_ptr_container : 
        less_than_comparable< reversible_ptr_container< Config, 
                                                          CloneAllocator >, 
        equality_comparable< reversible_ptr_container< Config, 
                                                         CloneAllocator > > >
    {
    private:
        BOOST_STATIC_CONSTANT( bool, allow_null = Config::allow_null );
        
        typedef BOOST_DEDUCED_TYPENAME Config::value_type Ty_;
        
        struct null_clone_allocator
        {
            static Ty_* allocate_clone( const Ty_* x )
            {
                if( allow_null )
                {
                    if( x == 0 )
                        return 0;
                }

                return CloneAllocator::allocate_clone( *x );
            }
            
            static void deallocate_clone( const Ty_* x )
            {
                if( allow_null )
                {
                    if( x == 0 )
                        return;
                }

                CloneAllocator::deallocate_clone( x );
            }
        };

        typedef BOOST_DEDUCED_TYPENAME Config::void_container_type Cont;
        typedef clone_deleter<null_clone_allocator>                Deleter;

        Cont      c_;

    protected:
        Cont& c_private()                { return c_; }
        const Cont& c_private() const    { return c_; }

    public: // typedefs
        typedef  BOOST_DEDUCED_TYPENAME Config::object_type  
                               object_type;
        typedef  Ty_*          value_type;
        typedef  Ty_*          pointer;
        typedef  Ty_&          reference;
        typedef  const Ty_&    const_reference;
        
        typedef  BOOST_DEDUCED_TYPENAME Config::iterator 
                                   iterator;
        typedef  BOOST_DEDUCED_TYPENAME Config::const_iterator
                                   const_iterator;
        typedef  BOOST_DEDUCED_TYPENAME Config::reverse_iterator
                                   reverse_iterator;
        typedef  BOOST_DEDUCED_TYPENAME Config::const_reverse_iterator     
                                   const_reverse_iterator;
        typedef  BOOST_DEDUCED_TYPENAME Cont::difference_type
                                   difference_type; 
        typedef  BOOST_DEDUCED_TYPENAME Cont::size_type
                                   size_type;
        typedef  BOOST_DEDUCED_TYPENAME Config::allocator_type
                                   allocator_type;

        typedef BOOST_DEDUCED_TYPENAME Config::ptr_iterator
                                   ptr_iterator; 
        typedef BOOST_DEDUCED_TYPENAME Config::ptr_const_iterator
                                   ptr_const_iterator;
        typedef BOOST_DEDUCED_TYPENAME Config::ptr_reverse_iterator
                                   ptr_reverse_iterator; 
        typedef BOOST_DEDUCED_TYPENAME Config::ptr_const_reverse_iterator
                                   ptr_const_reverse_iterator;

        typedef ptr_container_detail::static_move_ptr<Ty_,Deleter> 
                                   auto_type;
            
    protected: // implementation
            
        typedef ptr_container::ptr_container_detail::scoped_deleter<Ty_,null_clone_allocator>
                                   scoped_deleter;
    private:

        template< class InputIterator >  
        void copy( InputIterator first, InputIterator last ) 
        {
            std::copy( first, last, begin() );
        }
        
        void copy( const reversible_ptr_container& r )
        { 
            copy( r.begin(), r.end() );
        }
        
        void copy_clones_and_release( scoped_deleter& sd ) // nothrow
        {
            BOOST_ASSERT( size_type( std::distance( sd.begin(), sd.end() ) ) == c_.size() );
            std::copy( sd.begin(), sd.end(), c_.begin() );
            sd.release(); 
        }
        
        template< class OutputIterator >
        void copy_clones_and_release( scoped_deleter& sd, 
                                      OutputIterator where ) // nothrow
        {
            std::copy( sd.begin(), sd.end(), where );
            sd.release();
        }
        
        void insert_clones_and_release( scoped_deleter& sd ) // strong
        {
            c_.insert( sd.begin(), sd.end() );
            sd.release();
        }

        template< class ForwardIterator >
        void clone_assign( ForwardIterator first, 
                           ForwardIterator last ) // strong 
        {
            BOOST_ASSERT( first != last );
            scoped_deleter sd( first, last );      // strong
            copy_clones_and_release( sd );         // nothrow
        }

        template< class ForwardIterator >
        void clone_back_insert( ForwardIterator first,
                                ForwardIterator last )
        {
            BOOST_ASSERT( first != last );
            scoped_deleter sd( first, last );
            insert_clones_and_release( sd, end() );
        }
        
        /*
        void clone_assign( size_type n, const_reference x ) // strong
        {
            BOOST_ASSERT( n != 0 );
            scoped_deleter sd( n, deleter_ ); // strong     
            make_clones( sd, n, x );          // strong
            copy_clones_and_release( sd );    // nothrow
        }
        */

        /*
        void strong_resize_and_remove( size_type n )
        {
            const size_type old_size = c_.size();
            if( n > old_size ) // if resize won't throw
            {
                remove_all();
                c_.resize( n );
            }
            else if( n < old_size ) // if rezise might throw
            {
                size_undoer su( c_ );
                c_.resize( n );
                su.release();
                remove_all();
            }
            else 
            {
                BOOST_ASSERT( n == old_size );
                remove_all();
            }
        }
        */
        
        void remove_all() 
        {
            remove( begin(), end() ); 
        }

    protected:

        void insert_clones_and_release( scoped_deleter& sd, 
                                        iterator where ) // strong
        {
            //
            // 'c_.insert' always provides the strong guarantee for T* elements
            // since a copy constructor of a pointer cannot throw
            //
            c_.insert( where.base().base(), 
                       sd.begin(), sd.end() ); 
            sd.release();
        }

        template< class I >
        void remove( I i )
        { 
            null_policy_deallocate_clone( &*i );
        }

        template< class I >
        void remove( I first, I last ) 
        {
            for( ; first != last; ++first )
                remove( first );
        }

        template< class Range >
        BOOST_DEDUCED_TYPENAME range_result_iterator<Range>::type
        adl_begin( Range& r )
        {
            #if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))  
            return boost::begin( r );
            #else
            using boost::begin;
            return begin( r );
            #endif
        }

        template< class Range >
        BOOST_DEDUCED_TYPENAME range_result_iterator<Range>::type
        adl_end( Range& r )
        {
            #if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))  
            return boost::end( r );
            #else
            using boost::end;
            return end( r );
            #endif
        }
        
        static void enforce_null_policy( Ty_* x, const char* msg )
        {
            if( !allow_null )
            {
                if( 0 == x )
                    throw bad_pointer( msg );
            }
        }

        static Ty_* null_policy_allocate_clone( const Ty_* x )
        {
            return null_clone_allocator::allocate_clone( x );
        }

        static void null_policy_deallocate_clone( const Ty_* x )
        {
            null_clone_allocator::deallocate_clone( x );
        }

    private:
        template< class ForwardIterator >
        ForwardIterator advance( ForwardIterator begin, size_type n ) 
        {
            ForwardIterator iter = begin;
            std::advance( iter, n );
            return iter;
        }
        
    private:
        reversible_ptr_container( const reversible_ptr_container& );
        void operator=( const reversible_ptr_container& );
        
    public: // foundation! should be protected!
        explicit reversible_ptr_container( const allocator_type& a = allocator_type() ) 
         : c_( a )
        {}
        
        template< class PtrContainer >
        explicit reversible_ptr_container( std::auto_ptr<PtrContainer> clone )
          : c_( allocator_type() )                
        { 
            swap( *clone ); 
        }

    private:
        template< class I >
        void constructor_impl( I first, I last, std::input_iterator_tag ) // basic
        {
            while( first != last )
            {
                insert( end(), null_clone_allocator::allocate_clone( &*first ) );
                ++first;
            }
        }

        template< class I >
        void constructor_impl( I first, I last, std::forward_iterator_tag ) // strong
        {
            if( first == last )
                return;
            clone_back_insert( first, last );
        }


    public:
        // overhead: null-initilization of container pointer (very cheap compared to cloning)
        // overhead: 1 heap allocation (very cheap compared to cloning)
        template< class InputIterator >
        reversible_ptr_container( InputIterator first, 
                                    InputIterator last,
                                    const allocator_type& a = allocator_type() ) // basic, strong
        : c_( a )
        { 
            constructor_impl( first, last, BOOST_DEDUCED_TYPENAME
                              iterator_category<InputIterator>::type() );
        }

        template< class Compare >
        reversible_ptr_container( const Compare& comp,
                                    const allocator_type& a )
        : c_( comp, a ) {}

    public:        
        ~reversible_ptr_container()
        { 
            remove_all();
        }
        
        template< class PtrContainer >
        void operator=( std::auto_ptr<PtrContainer> clone )     
        {
            swap( *clone );
        }

    public:
        
        /*
        // overhead: 1 heap allocation (very cheap compared to cloning)
        void assign( size_type n, const_reference x ) // strong         
        {
            if( n == 0 )
                return;
            scoped_deleter sd( n, deleter_ ); // strong
            make_clones( sd, n, x );          // strong
            strong_resize_and_remove( n );    // strong
            copy_clones_and_release( sd );    // nothrow
        }
        */
               
        allocator_type get_allocator() const                   
        {
            return c_.get_allocator(); 
        }
 
    public: // container requirements
        iterator                   begin()            { return iterator( c_.begin() ); }
        const_iterator             begin() const      { return const_iterator( c_.begin() ); }
        iterator                   end()              { return iterator( c_.end() ); }
        const_iterator             end() const        { return const_iterator( c_.end() ); }
        reverse_iterator           rbegin()           { return reverse_iterator( c_.rbegin() ); } 
        const_reverse_iterator     rbegin() const     { return const_reverse_iterator( c_.rbegin() ); } 
        reverse_iterator           rend()             { return reverse_iterator( c_.rend() ); } 
        const_reverse_iterator     rend() const       { return const_reverse_iterator( c_.rend() ); } 
        ptr_iterator               ptr_begin()        { return c_.begin(); }
        ptr_const_iterator         ptr_begin() const  { return c_.begin(); }
        ptr_iterator               ptr_end()          { return c_.end(); }
        ptr_const_iterator         ptr_end() const    { return c_.end(); }
        ptr_reverse_iterator       ptr_rbegin()       { return c_.rbegin(); }
        ptr_const_reverse_iterator ptr_rbegin() const { return c_.rbegin(); }
        ptr_reverse_iterator       ptr_rend()         { return c_.rend(); }
        ptr_const_reverse_iterator ptr_rend() const   { return c_.rend(); }

        void swap( reversible_ptr_container& r ) // notrow
        { 
            c_.swap( r.c_ );
            // swap CloneAllocator ?
        }
          
        size_type size() const // nothrow
        {
            return c_.size();
        }

        /*
        void resize( size_type sz, const_reference x )  // strong     
        {
            size_type old_size = size();
            if( sz > old_size )
            {
                size_type n = sz - old_size;
                BOOST_ASSERT( n > 0 );
                scoped_deleter sd( n, deleter_ ); // strong
                make_clones( sd, n, x );          // strong
                size_undoer su( c_ );             // strong
                c_.resize( n );                   // strong or nothrow, commit
                su.release();                     // nothrow
                copy_clones_and_release( sd, advance( c_.begin(), old_size ) ); // nothrow
            }
            else if( sz < old_size )
            {
                erase( advance( begin(), sz ), end() ); // nothrow
            }
            else
                ;
        }
        */
        
        size_type max_size() const // nothrow
        {
            return c_.max_size(); 
        }
        
        bool empty() const // nothrow
        {
            return c_.empty();
        }

    public: // optional container requirements

        bool operator==( const reversible_ptr_container& r ) const // nothrow
        { 
            if( size() != r.size() )
                return false;
            else
                return std::equal( begin(), end(), r.begin() );
        }
        
        bool operator<( const reversible_ptr_container& r ) const // nothrow 
        {
             return std::lexicographical_compare( begin(), end(), r.begin(), r.end() );
        }
        
    public: // modifiers

        iterator insert( iterator before, Ty_* x )
        {
            enforce_null_policy( x, "Null pointer in 'insert()'" );

            auto_type ptr( x );                                   // nothrow
            iterator res( c_.insert( before.base().base(), x ) ); // strong, commit
            ptr.release();                                        // nothrow
            return res;
        };

        /*
        void insert( iterator before, size_type n, const_reference x ) // strong 
        {
            if( n == 0 )
                return;
            scoped_deleter sd( n, deleter_ );        // strong
            make_clones( sd, n, x );                 // strong 
            insert_clones_and_release( sd, before ); // strong, commit
        }
        */

        iterator erase( iterator x ) // nothrow 
        { 
            BOOST_ASSERT( !empty() );
            BOOST_ASSERT( x != end() );
            
            remove( x ); 
            return iterator( c_.erase( x.base().base() ) );
        }
        
        iterator erase( iterator first, iterator last ) // notrow 
        {
            BOOST_ASSERT( !empty() );
            remove( first, last ); 
            return iterator( c_.erase( first.base().base(), 
                                       last.base().base() ) );
        }

        template< class Range >
        iterator erase( const Range& r )
        {
            return erase( adl_begin(r), adl_end(r) );
        }
        
        void clear()                               
        { 
            remove_all(); 
            c_.clear();
        }
        
    public: // access interface
        
        auto_type release( iterator where )
        { 
            BOOST_ASSERT( where != end() );
            if( empty() )
                throw bad_ptr_container_operation( "'release()' on empty container" ); 
            
            auto_type ptr( &*where );                     // nothrow
            c_.erase( Config::get_base( where.base() ) ); // nothrow
            return move( ptr ); 
        }

        auto_type replace( iterator where, Ty_* x ) // strong  
        { 
            BOOST_ASSERT( where != end() );

            enforce_null_policy( x, "Null pointer in 'replace()'" );
            
            auto_type ptr( x );
            
            if( empty() )
                throw bad_ptr_container_operation( "'replace()' on empty container" );

            auto_type old( &*where );        // nothrow
            *where.base() = ptr.release();   // nothrow, commit
            return move( old );
        }

        /*
        auto_type replace( iterator where, auto_type ptr ) // strong  
        { 
            // hm.... move ptr problem..
        }
        */
        
        auto_type replace( size_type idx, Ty_* x ) // strong
        {
            enforce_null_policy( x, "Null pointer in 'replace()'" );
            
            auto_type ptr( x ); 
            
            if( idx >= size() ) 
                throw bad_index( "'replace()' out of bounds" );
            
            auto_type old( static_cast<Ty_*>( c_[idx] ) ); // nothrow
            c_[idx] = ptr.release();                       // nothrow, commit
            return move( old );
        } 
        
    }; // 'reversible_ptr_container'

    
    //
    // two-phase lookup of template functions 
    // is buggy on most compilers, so we use a macro instead
    //
#define BOOST_PTR_CONTAINER_DEFINE_RELEASE_AND_CLONE( PC, base_type ) \
                                                    \
    PC( std::auto_ptr<PC> r )                       \
    : base_type ( r ) { }                           \
                                                    \
    void operator=( std::auto_ptr<PC> r )           \
    {                                               \
        base_type::operator=( r );                  \
    }                                               \
                                                    \
    std::auto_ptr<PC> release()                     \
    {                                               \
      std::auto_ptr<PC> ptr( new PC );              \
      this->swap( *ptr );                           \
      return ptr;                                   \
    }                                               \
                                                    \
    using base_type::release;                       \
                                                    \
    std::auto_ptr<PC> clone() const                 \
    {                                               \
       return std::auto_ptr<PC>( new PC( this->begin(), this->end() ) ); \
    }

#define BOOST_PTR_CONTAINER_DEFINE_CONSTRUCTORS( PC, base_type )                     \
    typedef BOOST_DEDUCED_TYPENAME base_type::iterator        iterator;                \
    typedef BOOST_DEDUCED_TYPENAME base_type::size_type       size_type;               \
    typedef BOOST_DEDUCED_TYPENAME base_type::const_reference const_reference;         \
    typedef BOOST_DEDUCED_TYPENAME base_type::allocator_type  allocator_type;          \
    PC( const allocator_type& a = allocator_type() ) : base_type(a) {}                 \
    template< class InputIterator >                                                    \
    PC( InputIterator first, InputIterator last,                                       \
    const allocator_type& a = allocator_type() ) : base_type( first, last, a ) {}      
    

                 
#define BOOST_PTR_CONTAINER_DEFINE_NON_INHERITED_MEMBERS( PC, base_type )           \
   BOOST_PTR_CONTAINER_DEFINE_CONSTRUCTORS( PC, base_type )                         \
   BOOST_PTR_CONTAINER_DEFINE_RELEASE_AND_CLONE( PC, base_type )
    
    } // namespace 'ptr_container_detail'

    template< class Iterator >
    inline bool is_null( Iterator i )
    {
        return *i.base() == 0;
    }

} // namespace 'boost'  

#endif