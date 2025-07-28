#ifndef CORTADO_CONCEPTS_BACKGROUND_RESUMABLE_H
#define CORTADO_CONCEPTS_BACKGROUND_RESUMABLE_H

// Cortado
//
#include <Cortado/Concepts/CoroutineScheduler.h>
#include <Cortado/Concepts/TaskImpl.h>

namespace Cortado::Concepts
{

// Background resumable is a task implementation
// which defines a default thread pool onto which any
// task can be offloaded.
//
template <typename T>
concept BackgroundResumable = TaskImpl<T> && requires {
    { T::GetDefaultBackgroundScheduler() } -> CoroutineScheduler;
};

} // namespace Cortado::Concepts

#endif
