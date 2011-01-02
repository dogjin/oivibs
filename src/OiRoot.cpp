// =====================================================================================
// 
//       Filename:  OiRoot.cpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  2010-09-29 11:17:54
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  Sigitas Dagilis (), sigidagi@gmail.com
//   
//     This file is part of the OiVibrations C++ library.
//     It is provided without any warranty of fitness
//     for any purpose. You can redistribute this file
//     and/or modify it under the terms of the GNU
//     General Public License (GPL) as published
//     by the Free Software Foundation, either version 3
//     of the License or (at your option) any later version.
//     (see http://www.opensource.org/licenses for more info)
// 
// =====================================================================================

#include    "config.hpp"
#include	"OiRoot.h"
#include	"OiFileFormat.h"
#include	"OiProcessing.h"
#include	"OiUtil.h"
#include    <boost/foreach.hpp>

#define     foreach BOOST_FOREACH

namespace Oi
{

    Root* Root::instance_ = NULL;
    
    Root::Root()
    {

    }

    Root::~Root()
    {

    }

    Root& Root::operator=(Root const&)
    {
        return *this;
    }

    Root* Root::Instance()
    {
        if (!instance_)
            instance_ = new Root;
        
        return instance_;
    }

    
    // Implementation of ProxyInterface interface

    bool Root::init(int argc, const char** argv, int processName)
    {
        
        vector<string> fileList;
        for (int i = 0; i < argc; ++i)
        {
            // check if file format is supported.
            string strInFile = argv[i];
            string fileFormat = stripToExtension(strInFile);
            if (fileFormat != "uff" && fileFormat != "txt")
                continue;
           
            
            string::size_type idx = strInFile.rfind('\\');
            if (idx != string::npos)
            {
                string strPath = strInFile.substr(0, idx);
                
                chdir(strPath.c_str());
                strInFile = strInFile.substr(idx+1);
            }

            fileList.push_back(strInFile);
        }
        
        if (fileList.empty())
            return false;
             

        // Root is responsible for initialization of Storage
        // 
                
        foreach(string file, fileList)
        {
            shared_ptr<FileFormatInterface> fileFormat = FileFormatInterface::createFileFormat(this, file);
            fileFormat->parse();
            if (fileFormat->existNodes() || 
                fileFormat->existLines() ||
                fileFormat->existSurfaces() ||
                fileFormat->existRecords() )
            {
                fileFormatList_.push_back(fileFormat);
            }
            
            if (fileFormat->existRecords())
            {
                shared_ptr<ProcessingInterface> proc = ProcessingInterface::createProcess(processName);
                if (proc->start(fileFormat.get()))
                    procList_.push_back(proc);
 
            }

/*
 *            shared_ptr<ProcessingInterface> proc = ProcessingInterface::createProcess(this, processName);
 *            if (proc->start())
 *            {
 *                   procList_.push_back(proc);
 *
 *                   // save processed data, singular values, singular vectors and etc.
 *            }
 */
        }
        
        if (fileFormatList_.empty())
            return false;

        return true;
    }

    bool Root::connect(const string& file)
    {
/*
 *        string repoName = Oi::stripToBaseName(file);
 *        if (!storage_->existRepository(repoName))
 *            return false;
 *
 *        repositoryName_ = file;
 */

        return true;
    }
    
    double** Root::getNodes(int& size)
    {
        if (fileFormatList_.empty())
            return NULL;
       
        foreach(shared_ptr<FileFormatInterface> format, fileFormatList_)
        {
            if (format->existNodes())
            {
                const arma::mat& nodes = format->getNodes();
                size = (int)nodes.n_rows;
        
                return allocate2D(nodes); 
            }
        }

        return NULL;
    }

    unsigned int** Root::getLines(int& size)
    {
        if (fileFormatList_.empty())
            return NULL;
       
        foreach(shared_ptr<FileFormatInterface> format, fileFormatList_)
        {
            if (format->existLines())
            {
                const arma::umat& lines = format->getLines();
                size = (int)lines.n_rows;
        
                return allocate2D(lines); 
            }
        }

        return NULL;
    }
    
    unsigned int** Root::getSurfaces(int& size)
    {
        if (fileFormatList_.empty())
            return NULL;
       
        foreach(shared_ptr<FileFormatInterface> format, fileFormatList_)
        {
            if (format->existSurfaces())
            {
                const arma::umat& surfaces = format->getSurfaces();
                size = (int)surfaces.n_rows;
        
                return allocate2D(surfaces); 
            }
        }

        return NULL;

    }

    shared_ptr<FileFormatInterface> Root::getFileFormat() 
    {
        assert(false);
        return shared_ptr<FileFormatInterface>();
    }
   
    shared_ptr<ProcessingInterface> Root::getProcess(int i) 
    {
        if (i >= 0 && i < (int)procList_.size())
            return  procList_[i]; 
        else
            return shared_ptr<ProcessingInterface>();
    }


} // namespace Oi
