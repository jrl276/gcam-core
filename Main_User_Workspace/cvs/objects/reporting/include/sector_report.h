#ifndef _SECTOR_REPORT_H_
#define _SECTOR_REPORT_H_
#if defined(_MSC_VER)
#pragma once
#endif

/*
 * LEGAL NOTICE
 * This computer software was prepared by Battelle Memorial Institute,
 * hereinafter the Contractor, under Contract No. DE-AC05-76RL0 1830
 * with the Department of Energy (DOE). NEITHER THE GOVERNMENT NOR THE
 * CONTRACTOR MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY
 * LIABILITY FOR THE USE OF THIS SOFTWARE. This notice including this
 * sentence must appear on any copies of this computer software.
 * 
 * EXPORT CONTROL
 * User agrees that the Software will not be shipped, transferred or
 * exported into any country or used in any manner prohibited by the
 * United States Export Administration Act or any other applicable
 * export laws, restrictions or regulations (collectively the "Export Laws").
 * Export of the Software may require some form of license or other
 * authority from the U.S. Government, and failure to obtain such
 * export control license may result in criminal liability under
 * U.S. laws. In addition, if the Software is identified as export controlled
 * items under the Export Laws, User represents and warrants that User
 * is not a citizen, or otherwise located within, an embargoed nation
 * (including without limitation Iran, Syria, Sudan, Cuba, and North Korea)
 *     and that User is not otherwise prohibited
 * under the Export Laws from receiving the Software.
 * 
 * All rights to use the Software are granted on condition that such
 * rights are forfeited if User fails to comply with the terms of
 * this Agreement.
 * 
 * User agrees to identify, defend and hold harmless BATTELLE,
 * its officers, agents and employees from all liability involving
 * the violation of such Export Laws, either directly or indirectly,
 * by User.
 */

/*! 
* \file sector_report.h
* \ingroup Objects
* \brief SectorReport class header file.
*
*  Detailed description.
*
* \author Sonny Kim
*/

#include <string>
#include <iosfwd>
#include <memory>

#include "util/base/include/default_visitor.h"

class StorageTable;
class Sector;
class ProductionTechnology;

/*! 
* \ingroup Objects
* \brief CHANGE
* \details CHANGE
*
* \note CHANGE
* \author Sonny Kim
*/

class SectorReport : public DefaultVisitor {
public:
    SectorReport( std::ostream& aFile );
    void finish() const;
    void startVisitRegion( const Region* aRegion, const int aPeriod );
    void endVisitRegion( const Region* aRegion, const int aPeriod );
    void startVisitSector( const Sector* sector, const int aPeriod );
    void startVisitProductionTechnology( const ProductionTechnology* prodTechnology, const int period );
private:
    //! The file to which to write.
    std::ostream& mFile;
    std::auto_ptr<StorageTable> mTable; //!< Internal storage table for the SectorReport.
    std::string mSectorName; //!< sector name

    //! The current region name.
    std::string mCurrRegion;

    static const std::string getYearString( const int aYear );
};

#endif // _SECTOR_REPORT_H_

