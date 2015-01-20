////
//// pd_container.h
////
//// Copyright (c) 2012 - 2015 Karl-Johan Alm (http://github.com/kallewoof)
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
//#ifndef INCLUDED_PD_CONTAINER_H
//#define INCLUDED_PD_CONTAINER_H
//
//#include "pd_stack.h"
//#include "PDReference.h"
//#include "pd_dict.h"
//#include "pd_array.h"
//
///**
// *  Determine the type of the given stack.
// *
// *  This is a helper function for pd_dict and pd_array.
// *
// *  @param stack PDF implementation compliant stack
// *
// *  @return PDObjectType enum value
// */
//static inline PDObjectType pd_container_determine_type(pd_stack stack)
//{
//    PDObjectType type = PDObjectTypeNull;
//    if (stack) {
//        if (stack->type == PD_STACK_ID) {
//            PDID id = stack->info;
//            if (PDIdentifies(id, PD_DICT)) {
//                type = PDObjectTypeDictionary;
//            }
//            else if (PDIdentifies(id, PD_ARRAY)) {
//                type = PDObjectTypeArray;
//            }
//            else if (PDIdentifies(id, PD_NAME)) {
//                type = PDObjectTypeName;
//            }
//            else if (PDIdentifies(id, PD_REF)) {
//                type = PDObjectTypeReference;
//            }
//            else if (PDIdentifies(id, PD_HEXSTR)) {
//                type = PDObjectTypeString;
//            }
//            else {
//                PDNotice("unknown stack identity symbol encountered");
//                type = PDObjectTypeUnknown;
//            }
//        } else if (stack->type == PD_STACK_STRING) {
//            type = PDObjectTypeString;
//        } else {
//            PDNotice("unparseable stack type %d encountered", stack->type);
//        }
//    }
//    
//    return type;
//}
//
///**
// *  Spawn the appropriate object from the given stack.
// *
// *  @see pd_dict, pd_array
// *
// *  @param stack PDF implementation compliant stack
// *
// *  @return Appropriate object
// */
//static inline void *pd_container_spawn(pd_stack stack)
//{
//    if (stack) {
//        if (stack->type == PD_STACK_ID) {
//            PDID id = stack->info;
//            if (PDIdentifies(id, PD_DICT)) {
//                return pd_dict_from_pdf_dict_stack(stack);
//            }
//            if (PDIdentifies(id, PD_ARRAY)) {
//                return pd_array_from_pdf_array_stack(stack);
//            }
//            if (PDIdentifies(id, PD_NAME)) {
//                return strdup(stack->prev->info);
//            }
//            if (PDIdentifies(id, PD_REF)) {
//                return PDReferenceCreateFromStackDictEntry(stack);
//            }
//            if (PDIdentifies(id, PD_HEXSTR)) {
//                char *buf = malloc(3 + strlen(stack->prev->info));
//                sprintf(buf, "<%s>", stack->prev->info);
//                return buf; // strdup(stack->prev->info);
//            }
//            return NULL;
//        }
//        if (stack->prev) {
//            // it's a short hand array
//            return pd_array_from_stack(stack);
//        }
//        if (stack->type == PD_STACK_STRING) {
//            return strdup(stack->info);
//        }
//    }
//    return NULL;
//}
//
//#endif
