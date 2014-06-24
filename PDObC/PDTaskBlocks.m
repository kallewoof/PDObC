//
// PDTaskBlocks.m
//
// Copyright (c) 2012 - 2014 Karl-Johan Alm (http://github.com/kallewoof)
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#import "PDTaskBlocks.h"
#import "pd_internal.h"

extern void PDTaskDealloc(void *ob);

PDTaskResult PDBlockLauncher(PDPipeRef pipe, PDTaskRef task, PDObjectRef object, void *info)
{
    return as(__bridge PDTaskBlock, task->info)(pipe, task, object);
}

void PDBlockTaskDealloc(void *ob) 
{
    CFBridgingRelease(as(PDTaskRef, ob)->info);
    PDTaskDealloc(ob);
}

PDTaskRef PDTaskCreateBlockMutator(PDTaskBlock mutatorBlock)
{
    PDTaskRef task = PDTaskCreateMutator(PDBlockLauncher);
    task->deallocator = PDBlockTaskDealloc;
    task->info = (void*)CFBridgingRetain([mutatorBlock copy]);
    return task;
}

