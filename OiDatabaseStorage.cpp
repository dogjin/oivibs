// =====================================================================================
// 
//       Filename:  DatabaseStorage.cpp
// 
//    Description:  DatabaseStorage
// 
//        Version:  1.0
//        Created:  2010-06-15 14:02:38
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  Sigitas Dagilis (), sigidagi@gmail.com
//        Company:  sigidagis
// 
// =====================================================================================

#include "OiDatabaseStorage.h"
#include "OiFileFormat.h"
#include "OiProcessing.h"
#include "OiUtil.h"

#include <iostream>
#include <stdexcept>


namespace Oi {

    DatabaseStorage* DatabaseStorage::instance_ = NULL;
    
    DatabaseStorage::DatabaseStorage() : fileFormat_(NULL), proc_(NULL)
    {

    }

    DatabaseStorage::~DatabaseStorage()
    {
        if (proc_)
        {
            delete proc_;
            proc_ = NULL;
        }
        if (fileFormat_)
        {
            delete fileFormat_;
            fileFormat_ = NULL;
        }
    }

    DatabaseStorage& DatabaseStorage::operator=(DatabaseStorage const&)
    {
        return *this;
    }

    DatabaseStorage* DatabaseStorage::Instance()
    {
        if (!instance_)
            instance_ = new DatabaseStorage;
        
        return instance_;
    }

    bool DatabaseStorage::isConnected()
    {
        return bConnected_;
    }

    mysqlpp::Connection& DatabaseStorage::getConnection()
    {
        return connection_;
    }


    /* *
     * init function creates database with specified name and makes a connection
     * which is stored in private variable mysqlpp::Connection connection_
     *
     * Second phase is to parse given file and store found data: nodes, lines, surfaces and recorded data 
     * into that database.
     *
     */
    bool DatabaseStorage::init(const string& file, int processName)
    {
        if (file.empty())
            return false;
   
   
        string baseName = Oi::stripToBaseName(file);
        if (baseName.empty())
            return false;

        // noexception is needed not to throw exception when such database exist.
        mysqlpp::NoExceptions ne(connection_);
        
        if (connect(baseName))
        {
            std::cout << "Dropping existing sample data tables..." << std::endl;
            connection_.drop_db(baseName);
        }
        
        if (!connection_.create_db(baseName))
        {
            std::cerr << "Error creating DB: " << connection_.error() << std::endl; 
            return false;
        }

        if (!connection_.select_db(baseName))
        {
            std::cerr << "Error selecting DB: " << connection_.error() << std::endl;
            return false;
        }
       
        // 
        fileFormat_ = FileFormatInterface::createFileFormat(file);
        if (fileFormat_ == 0)
            return false;

        fileFormat_->parse(file);

        if (fileFormat_->existNodes())
        {
            const arma::mat& nodes = fileFormat_->getNodes(); 
            saveNodes(nodes);
        }
        if (fileFormat_->existLines())
        {
            saveLines(fileFormat_->getLines());
        }
        if (fileFormat_->existSurfaces())
        {
            saveSurfaces(fileFormat_->getSurfaces());
        }
        if (fileFormat_->existData())
        {
            
            proc_ = ProcessingInterface::createProcess(processName);
            if (proc_->start(fileFormat_))
                saveData(fileFormat_->getData());
               
            delete proc_;
            proc_ = 0;
        }
 
        /*
         *
         *if (!fileFormat_->existNodes() && !fileFormat_->existLines() && !fileFormat_->existSurfaces() && !fileFormat_->existData())
         *{
         *    delete fileFormat_;
         *    fileFormat_ = 0;
         *    return false;
         *}
         */
       
        delete fileFormat_;
        fileFormat_ = 0;

        return true;
    }

    bool DatabaseStorage::connect(const string& file)
    {

        string dbname = Oi::stripToBaseName(file);
        if (dbname.empty())
            return false;

        const char *server = 0, *user = "root", *pass = "testas";

        try 
        {
            if (!connection_.connect(0, server, user, pass))
            {
                return false;
            }
        }
        catch (std::exception& er) 
        {
            return false;
        }

        mysqlpp::NoExceptions ne(connection_);
        if (!connection_.select_db(dbname))
            return false;

        bConnected_ = true;
        dbname_ = dbname;

        return true;
    }

    bool DatabaseStorage::createTableOfLines()
    {
        mysqlpp::Query query = connection_.query();

        try
        {
            std::cout << "Creating empty lines table..." << std::endl;
            query << 
                "CREATE TABLE geolines (" <<
                " id INT UNSIGNED AUTO_INCREMENT, " << 
                " node1 INT NOT NULL, " <<
                " node2 INT NOT NULL, " <<
                " PRIMARY KEY (id)" << 
                ")";

            query.execute();
        }
        catch( std::exception& er)
        {
            std::cerr << "Query error: " << er.what() << "std::endl";
            return false;
        }

        std::cout << "Succeeded!\n";
        std::cout << "\n";
        return true;
    }

    bool DatabaseStorage::createTableOfSurfaces()
    {
        mysqlpp::Query query = connection_.query();
        
        try
        {
            std::cout << "Creating empty surfaces table..." << std::endl;
            query << 
                "CREATE TABLE geosurfaces (" <<
                " id INT UNSIGNED AUTO_INCREMENT, "
                " node1 INT NOT NULL, " <<
                " node2 INT NOT NULL, " <<
                " node3 INT NOT NULL, " << 
                " PRIMARY KEY (id)" <<
                ")";
            query.execute();

        }
        catch (std::exception& er)
        {
            std::cerr << "Query error: " << er.what() << "std::endl";
            return false;
        }

        std::cout << "Succeeded!\n";
        std::cout << "\n";
        return true;
    }

    bool DatabaseStorage::createTableOfNodes()
    {
        mysqlpp::Query query = connection_.query();

        // Creating tables;
        try
        {
            std::cout << "Creating Node table...\n";
            query << "CREATE TABLE geonodes (" <<
                " id INT NOT NULL, " <<
                " PRIMARY KEY(id), " <<
                " x DOUBLE, " << 
                " y DOUBLE, " <<
                " z DOUBLE " <<
                ")" <<
                "ENGINE = InnoDB " <<
                "CHARACTER SET utf8 COLLATE utf8_general_ci";

            query.execute();
        }
        catch (std::exception& er)
        {
            std::cerr << "Query error: " << er.what() << "std::endl";
            return false;
        }
        
        std::cout << "Succeeded!\n";
        std::cout << "\n";
        return true;
    }

    // Function creates MySql tables according UFF file.
    bool DatabaseStorage::createTableOfData()
    {
        mysqlpp::Query query = connection_.query();
        
        try
        {
            std::cout << "Creating data table..." << std::endl;
            query.reset();
            query <<
                "CREATE TABLE measurement (" <<
        //		" id INT UNSIGNED AUTO_INCREMENT, " <<
                " label CHAR(30) NOT NULL, " <<
                " cdate CHAR(30), " <<			// creation date
                " ftype INT UNSIGNED, " <<		// function type must be 1 - time response
                " rnode INT UNSIGNED, " <<		// response node 
                " rdirection INT UNSIGNED, " << // response direction
                " dtype INT UNSIGNED, " <<		// data type must be 2 or 4 real single or double precision
                " dpoints INT UNSIGNED, " <<	// data points 
                " sinterval DOUBLE, " <<		// sampling interval
                " units CHAR(10), " <<			// measurement units
                " bdata BLOB " <<				// binary data
                ")";
            query.execute();

        }
        catch (std::exception& er)
        {
            std::cerr << "Query error: " << er.what() << "std::endl";
            return false;
        }
        
        std::cout << "Succeeded!\n";
        std::cout << "\n";
        return true;
    }

    void DatabaseStorage::saveNodes(const arma::mat& nodes)
    {
        if (nodes.n_elem == 0 || nodes.n_rows != 4)
            return;
        
        createTableOfNodes();

        mysqlpp::Query query = getConnection().query();
        query  << "insert into %4:table values" <<
            "(%0, %1, %2, %3)";
        query.parse();

        query.template_defaults["table"] = "geonodes";
       
        for (size_t i = 0; i < nodes.n_cols; ++i)
        {
            query.execute(nodes(0,i), nodes(1,i), nodes(2,i), nodes(3,i));
        }
          
    }

    void DatabaseStorage::saveLines(const arma::umat& lines)
    {
        if (lines.n_elem == 0 || lines.n_rows != 3)
            return;

        createTableOfLines();

        mysqlpp::Query query = getConnection().query();
        query  << "insert into %3:table values" <<
            "(%0, %1, %2)";
        query.parse();

        query.template_defaults["table"] = "geolines";

        for (size_t i = 0; i < lines.n_cols; ++i)
        {
            query.execute(lines(0,i), lines(1,i), lines(2,i));
        }
    }

    void DatabaseStorage::saveSurfaces(const arma::umat& surfaces)
    {
        if (surfaces.n_elem == 0 || surfaces.n_rows != 4)
            return;

        createTableOfSurfaces();

        mysqlpp::Query query = getConnection().query();
        query  << "insert into %4:table values" <<
            "(%0, %1, %2, %3)";
        query.parse();

        query.template_defaults["table"] = "geosurfaces";

        for (size_t i = 0; i < surfaces.n_cols; ++i)
        {
            query.execute(surfaces(0,i), surfaces(1,i), surfaces(2,i), surfaces(3,i));
        }
    }

    void DatabaseStorage::saveData(const arma::mat& data)
    {

    }
    
    // Implementation of ProxyInterface interface

    double** DatabaseStorage::getNodes(int& size)
    {
        if (!connect(dbname_))
            return NULL;    

        mysqlpp::Query query = connection_.query("select * from geonodes");
        mysqlpp::StoreQueryResult res = query.store();
       
        size = (int)res.num_rows(); 
        if (size == 0)
            return NULL;
        
        // allocation of memory of 2D array
        // Dealocation of memory should be carried out by CLIENT! 
        double** array = new double*[size];
        for (int n = 0; n < size; ++n)
            array[n] = new double[3];

        // assign values
        for (size_t i = 0; i < res.num_rows(); ++i)
        {
            array[i][0] = (res[i]["x"]);
            array[i][1] = (res[i]["y"]);
            array[i][2] = (res[i]["z"]);
        }
        
        return array;
    }

    double** DatabaseStorage::getLines(int& size)
    {
        bool bRet = connect(dbname_);
        if (bRet == false)
            return NULL;

        mysqlpp::Query query_nodes = connection_.query("select * from geonodes");
        mysqlpp::StoreQueryResult resNodes = query_nodes.store(); 

        mysqlpp::Query query_lines = connection_.query("select * from geolines");
        mysqlpp::StoreQueryResult resLines = query_lines.store();
        
        
        if (!resLines || !resNodes)
            return NULL;

        size = (int)resLines.num_rows();

        // allocation memeory
        //
        double** array = new double*[size];
        for (int n = 0; n < size; ++n)
            array[n] = new double[6];

        int node1, node2;
        
        for (int idx = 0; idx < size; ++idx)
        {
            node1 = resLines[idx]["node1"];
            array[idx][0] = resNodes[node1-1]["x"];
            array[idx][1] = resNodes[node1-1]["y"];
            array[idx][2] = resNodes[node1-1]["z"];
            
            node2 = resLines[idx]["node2"];
            array[idx][3] = resNodes[node2-1]["x"];
            array[idx][4] = resNodes[node2-1]["y"];
            array[idx][5] = resNodes[node2-1]["z"];
        }
       
        return array;
    }

    double** DatabaseStorage::getSurfaces(int& size)
    {
            bool bRet = connect(dbname_);
            if (bRet == false)
                return NULL;

        
            mysqlpp::Query query = connection_.query("select * from surfaces");
            mysqlpp::StoreQueryResult resSurfaces = query.store();
            if (!resSurfaces)
                return NULL;

            query.reset();
            query = connection_.query("select * from geonodes");
            mysqlpp::StoreQueryResult resNodes = query.store(); 
            if (!resNodes)
                return NULL;

            
            size = (int)resSurfaces.num_rows();

            // memory allocation
            double** array = new double*[size];
            for (int n = 0; n < size; ++n)
                array[n] = new double[9];

            int node1, node2, node3;
           
           for (int idx = 0; idx < size; ++idx)
            {
                node1 = resSurfaces[idx]["node1"];
                array[idx][0] = resNodes[node1-1]["x"];
                array[idx][1] = resNodes[node1-1]["y"];
                array[idx][2] = resNodes[node1-1]["z"];
     
                node2 = resSurfaces[idx]["node2"];
                array[idx][3] = resNodes[node2-1]["x"];
                array[idx][4] = resNodes[node2-1]["y"];
                array[idx][5] = resNodes[node2-1]["z"];
                
                node3 = resSurfaces[idx]["node3"];
                array[idx][6] = resNodes[node3-1]["x"];
                array[idx][7] = resNodes[node3-1]["y"];
                array[idx][8] = resNodes[node3-1]["z"];
            }
            
            return array;
    }

} // namespace Oi