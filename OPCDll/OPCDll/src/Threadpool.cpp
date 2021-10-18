/*************************************************************************
*
* VULCANFORMS CONFIDENTIAL
* __________________
*  Copyright, VulcanForms Inc.
*  [2016] - [2020] VulcanForms Incorporated
*  All Rights Reserved.
*
*  "VulcanForms", "Vulcan", "Fusing the Future"
*       are trademarks of VulcanForms, Inc.
*
* NOTICE:  All information contained herein is, and remains
* the property of VulcanForms Incorporated and its suppliers,
* if any.  The intellectual and technical concepts contained
* herein are proprietary to VulcandForms Incorporated
* and its suppliers and may be covered by U.S. and Foreign Patents,
* patents in process, and are protected by trade secret or copyright law.
* Dissemination of this information or reproduction of this material
* is strictly forbidden unless prior written permission is obtained
* from VulcanForms Incorporated.
*/
#pragma once

#include "threadpool.h"

Threadpool::Threadpool(unsigned int nThreads) :mPoolThreads(nThreads), mbStopnow(false), mbStopped(false)
{
    mHardThreads = std::thread::hardware_concurrency();
    //mPoolThreads = mHardThreads - 2;
    if (mPoolThreads > mHardThreads) {
        mPoolThreads = mHardThreads - 2;
    }

    for (unsigned int i=0; i < mPoolThreads; ++i) {
        mPool.push_back(std::thread(&Threadpool::infinite_loop,this));
    }
}

Threadpool::~Threadpool()
{
    shutdown();
}

void Threadpool::infinite_loop()
{
    do {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(mQMutex);
            mJobAlert.wait(lock, [this] {return !mQJobs.empty() || mbStopnow;});
			if (mbStopnow) break;
            job = mQJobs.front();
            mQJobs.pop();
        }
        job();
    } while (true);
}

void Threadpool::add_job(std::function<void()> new_job)
{
    {
        std::unique_lock<std::mutex> lock(mQMutex);
        mQJobs.push(new_job);
    }
    mJobAlert.notify_one();
}

void Threadpool::shutdown()
{
    {
        std::unique_lock<std::mutex> lock(mPMutex);
        mbStopnow = true;
        mbStopped = true;

        mJobAlert.notify_all();
    }

    {
        size_t nThreads = mPool.size();
        for (auto itr = mPool.begin(); itr != mPool.end(); ++itr)
        {
            if (itr->joinable()) {
                itr->join();
            }
        }
        mPool.clear();
    }
}

