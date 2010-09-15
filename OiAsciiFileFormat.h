#ifndef _OIASCIIFILEFORMAT_H
#define _OIASCIIFILEFORMAT_H

#include "OiFileFormat.h"
#include <armadillo>
#include <string>
#include <fstream>

using std::string;

namespace Oi {

    class AsciiFileFormat : public FileFormatInterface  
    {
        public:
            AsciiFileFormat();
            ~AsciiFileFormat(){}

        public:
            virtual void parse(const string& file);
            
            virtual bool existNodes();
            virtual bool existLines();
            virtual bool existSurfaces();
            virtual bool existData();

            virtual const arma::mat& getNodes();
            virtual const arma::umat& getLines();
            virtual const arma::umat& getSurfaces();
            virtual const arma::mat& getData();

            virtual double getSamplingInterval();
            virtual int getNumberOfSamples();

        private:
            
            std::ifstream fileStream_;
            arma::mat nodes_;
            arma::umat lines_;
            arma::umat surfaces_;
            arma::mat data_;

            double samplingInterval_;
            int numberOfSamples_;

            bool existData_;
            bool existSamplingInterval_;

        private:
            bool searchSamplingInterval(const string& fileName);
            
    };

} // namespace Oi

#endif