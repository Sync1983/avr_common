/*
    2     RingBufferT.h - A ring buffer template class for AVR processors.
    3     For AVR ATMega328p (Arduino Uno) and ATMega2560 (Arduino Mega).
    4     This is part of the AVRTools library.
    5     Copyright (c) 2014 Igor Mikolic-Torreira.  All right reserved.
    6 
    7     This program is free software: you can redistribute it and/or modify
    8     it under the terms of the GNU General Public License as published by
    9     the Free Software Foundation, either version 3 of the License, or
   10     (at your option) any later version.
   11 
   12     This program is distributed in the hope that it will be useful,
   13     but WITHOUT ANY WARRANTY; without even the implied warranty of
   14     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   15     GNU General Public License for more details.
   16 
   17     You should have received a copy of the GNU General Public License
   18     along with this program.  If not, see <http://www.gnu.org/licenses/>.
   19 */
   20 
   21 
   22 
   23 
   35 #ifndef RingBufferT_h
   36 #define RingBufferT_h
   37 
   38 
   39 #include <util/atomic.h>
   40 
   41 
   60 template< typename T, typename N, unsigned int SIZE > class RingBufferT
   61 {
   62 
   63 public:
   64 
   69     RingBufferT()
   70         : mSize( SIZE ), mLength( 0 ), mIndex( 0 )
   71         {}
   72 
   73 
   83     T pull()
   84     {
   85         T element = 0;
   86         ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
   87         {
   88             if ( mLength )
   89             {
   90                 element = mBuffer[ mIndex ];
   91                 mIndex++;
   92                 if ( mIndex >= mSize )
   93                 {
   94                     mIndex -= mSize;
   95                 }
   96                 --mLength;
   97             }
   98         }
   99         return element;
  100     }
  101 
  102 
  115     T peek( N index = 0 )
  116     {
  117         T element;
  118         ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
  119         {
  120             element = mBuffer[ ( mIndex + index ) % mSize ];
  121         }
  122         return element;
  123     }
  124 
  125 
  134     bool push( T element )
  135     {
  136         ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
  137         {
  138             if ( mLength < mSize )
  139             {
  140                 mBuffer[ ( mIndex + mLength ) % mSize ] = element;
  141                 ++mLength;
  142                 return 0;
  143             }
  144         }
  145         // True = failure
  146         return 1;
  147     }
  148 
  149 
  155     bool isEmpty()
  156     {
  157         return !static_cast<bool>( mLength );
  158     }
  159 
  160 
  166     bool isNotEmpty()
  167     {
  168         return static_cast<bool>( mLength );
  169     }
  170 
  171 
  177     bool isFull()
  178     {
  179         ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
  180         {
  181             return ( mSize - mLength ) <= 0;
  182         }
  183     }
  184 
  185 
  191     bool isNotFull()
  192     {
  193         ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
  194         {
  195             return ( mSize - mLength ) > 0;
  196         }
  197     }
  198 
  199 
  205     void discardFromFront( N nbrElements )
  206     {
  207         ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
  208         {
  209             if ( nbrElements < mLength )
  210             {
  211                 mIndex += nbrElements;
  212                 if( mIndex >= mSize )
  213                 {
  214                     mIndex -= mSize;
  215                 }
  216                 mLength -= nbrElements;
  217             }
  218             else
  219             {
  220                 // flush the whole buffer
  221                 mLength = 0;
  222             }
  223         }
  224     }
  225 
  226 
  230     void clear()
  231     {
  232         ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
  233         {
  234             mLength = 0;
  235         }
  236     }
  237 
  238 
  239 
  240 private:
  241 
  242     T mBuffer[ SIZE ] ;
  243     volatile N mSize;
  244     volatile N mLength;
  245     volatile N mIndex;
  246 
  247 };
  248 
  249 
  250 #endif