
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
#ifndef DEF_THREADPOOL_H
#define DEF_THREADPOOL_H

#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>

class Threadpool
{
public:
    Threadpool(unsigned int nThreads = 5);
    ~Threadpool();

    void infinite_loop();

    void add_job(std::function<void()> new_job);
    bool has_jobs()
    {
        bool bHas = false; 
        { 
            std::lock_guard<std::mutex> lock(mQMutex);
            bHas = !mQJobs.empty(); 
        } 
        return bHas;
    }
    size_t num_jobs()
    {
        size_t items = 0;
        {
            std::lock_guard<std::mutex> lock(mQMutex);
            items = mQJobs.size();
        }
        return items; 
    }
    void shutdown();

private:
    unsigned int mHardThreads;
    unsigned int mPoolThreads;
    std::vector<std::thread> mPool;
    std::queue<std::function<void()>> mQJobs;
    std::mutex mQMutex;
    std::mutex mPMutex;
    std::condition_variable mJobAlert;
    bool mbStopnow;
    bool mbStopped;
};
#endif

