////
//// PDCollection.c
////
//// Copyright (c) 2012 - 2014 Karl-Johan Alm (http://github.com/kallewoof)
//// 
//// This program is free software: you can redistribute it and/or modify
//// it under the terms of the GNU General Public License as published by
//// the Free Software Foundation, either version 3 of the License, or
//// (at your option) any later version.
//// 
//// This program is distributed in the hope that it will be useful,
//// but WITHOUT ANY WARRANTY; without even the implied warranty of
//// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//// GNU General Public License for more details.
//// 
//// You should have received a copy of the GNU General Public License
//// along with this program.  If not, see <http://www.gnu.org/licenses/>.
////
//
//#include "Pajdeg.h"
//#include "PDCollection.h"
//#include "pd_stack.h"
//#include "pd_array.h"
//#include "pd_dict.h"
//
//#include "pd_internal.h"
//
//void PDCollectionDestroy(PDCollectionRef collection)
//{
//    pd_stack s;
//    
//    switch (collection->type) {
//        case PDCollectionTypeArray:
//            pd_array_destroy(collection->data);
//            break;
//            
//        case PDCollectionTypeDictionary:
//            pd_dict_destroy(collection->data);
//            break;
//            
//        case PDCollectionTypeStack:
//            s = collection->data;
//            pd_stack_destroy(&s);
//            
//        default:
//            PDError("invalid type encountered for PDCollection: %d", collection->type);
//            PDAssert(0);
//    }
//}
//
//PDCollectionRef PDCollectionCreateWithData(void *data, PDCollectionType type)
//{
//    PDCollectionRef collection = PDAlloc(sizeof(struct PDCollection), PDCollectionDestroy, false);
//    collection->type = type;
//    collection->data = data;
//    return collection;
//}
//
//PDCollectionRef PDCollectionCreateWithArray(pd_array array)
//{
//    return PDCollectionCreateWithData(array, PDCollectionTypeArray);
//}
//
//PDCollectionRef PDCollectionCreateWithDictionary(pd_dict dict)
//{
//    return PDCollectionCreateWithData(dict, PDCollectionTypeDictionary);
//}
//
//PDCollectionRef PDCollectionCreateWithStack(pd_stack stack)
//{
//    return PDCollectionCreateWithData(stack, PDCollectionTypeStack);
//}
//
//PDCollectionType PDCollectionGetType(PDCollectionRef collection)
//{
//    return collection->type;
//}
//
//pd_array PDCollectionGetArray(PDCollectionRef collection)
//{
//    PDAssert(collection->type == PDCollectionTypeArray);
//    return collection->data;
//}
//
//pd_dict PDCollectionGetDictionary(PDCollectionRef collection)
//{
//    PDAssert(collection->type == PDCollectionTypeDictionary);
//    return collection->data;
//}
//
//pd_stack PDCollectionGetStack(PDCollectionRef collection)
//{
//    PDAssert(collection->type == PDCollectionTypeStack);
//    return collection->data;
//}
