// =====================================================================================
// 
//       Filename:  OiProxy.h
// 
//    Description:  Proxy class for object decoupling
// 
//        Version:  1.0
//        Created:  2010-09-06 16:26:16
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  Sigitas Dagilis (), sigidagi@gmail.com
//        Company:  sigidagis
// 
// =====================================================================================

#ifndef _OIPROXY_H
#define _OIPROXY_H

#include <string>

using std::string;

#define DLL __atribute__(dllexport)

/*
 *#ifdef __cplusplus
 *extern "C"
 *{
 *#endif
 *
 *namespace Oi
 *{
 *
 *} // namespace Oi
 *
 *#ifdef __cplusplus
 *}
 *#endif
 */

namespace Oi {

    class ProxyInterface
    {
        public:
            virtual ~ProxyInterface(){}
        
        // Interfaces
        public:
            virtual bool init(const string& name, int processName = 0) = 0;
            virtual bool connect(const string& name) = 0;
            virtual double** getNodes(int& ) = 0;
            virtual double** getLines(int& ) = 0;
            virtual double** getSurfaces(int& ) = 0;
    };

    class Proxy : ProxyInterface {
        public:
            Proxy(); 
            ~Proxy ();

        // Interfaces
        public:
            bool init(const string& name, int processName = 0);
            bool connect(const string& name);
            double** getNodes(int& size);
            double** getLines(int& size);
            double** getSurfaces(int& size);

        private:
            ProxyInterface* impl_; 
    };

} // namespace Oi

#endif