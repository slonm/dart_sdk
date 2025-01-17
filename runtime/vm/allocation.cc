// Copyright (c) 2012, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/allocation.h"

#include "platform/assert.h"
#include "vm/isolate.h"
#include "vm/thread.h"
#include "vm/zone.h"

namespace dart {

static void* Allocate(uword size, Zone* zone) {
  ASSERT(zone != nullptr);
  if (size > static_cast<uword>(kIntptrMax)) {
    FATAL("ZoneAllocated object has unexpectedly large size %" Pu "", size);
  }
  return reinterpret_cast<void*>(zone->AllocUnsafe(size));
}

void* ZoneAllocated::operator new(uword size) {
  return Allocate(size, Thread::Current()->zone());
}

void* ZoneAllocated::operator new(uword size, Zone* zone) {
  return Allocate(size, zone);
}

StackResource::~StackResource() {
  if (thread_ != nullptr) {
    StackResource* top = thread_->top_resource();
    ASSERT(top == this);
    thread_->set_top_resource(previous_);
  }
#if defined(DEBUG)
  if (thread_ != nullptr) {
    ASSERT(Thread::Current() == thread_);
  }
#endif
}

void StackResource::Init(ThreadState* thread) {
  // We can only have longjumps and exceptions when there is a current
  // thread and isolate.  If there is no current thread, we don't need to
  // protect this case.
  // TODO(23807): Eliminate this special case.
  if (thread != nullptr) {
    ASSERT(Thread::Current() == thread);
    thread_ = thread;
    previous_ = thread_->top_resource();
    ASSERT((previous_ == nullptr) || (previous_->thread_ == thread));
    thread_->set_top_resource(this);
  }
}

void StackResource::UnwindAbove(ThreadState* thread, StackResource* new_top) {
  StackResource* current_resource = thread->top_resource();
  while (current_resource != new_top) {
    current_resource->~StackResource();
    current_resource = thread->top_resource();
  }
}

}  // namespace dart
