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
* \file sector.cpp
* \ingroup Objects
* \brief Sector class source file.
* \author Sonny Kim, Steve Smith, Josh Lurz
*/

#include "util/base/include/definitions.h"
#include <string>
#include <fstream>
#include <cassert>
#include <algorithm>
#include <stack>

// xml headers
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>

#include "util/base/include/xml_helper.h"
#include "sectors/include/more_sector_info.h"
#include "sectors/include/sector.h"
#include "sectors/include/subsector.h"
#include "containers/include/scenario.h"
#include "util/base/include/model_time.h"
#include "marketplace/include/marketplace.h"
#include "util/base/include/configuration.h"
#include "util/base/include/summary.h"
#include "containers/include/world.h"
#include "util/base/include/util.h"
#include "util/logger/include/ilogger.h"
#include "containers/include/info_factory.h"
#include "util/logger/include/logger.h"
#include "containers/include/national_account.h"
#include "containers/include/iinfo.h"
#include "util/base/include/ivisitor.h"
#include "reporting/include/sector_report.h"
#include "sectors/include/tran_subsector.h"
#include "sectors/include/sector_utils.h"
#include "util/base/include/input_finder.h"

using namespace std;
using namespace xercesc;

extern Scenario* scenario;

/*! \brief Default constructor.
*
* Constructor initializes member variables with default values, sets vector
* sizes, and sets value of debug flag.
*
* \author Sonny Kim, Steve Smith, Josh Lurz
*/
Sector::Sector( const string& aRegionName )
    : regionName( aRegionName ),
      mObjectMetaInfo()
{
    mSectorType = getDefaultSectorType();
    mBaseOutput = 0;
    mBasePrice = 0;

    // resize vectors
    const Modeltime* modeltime = scenario->getModeltime();
    const int maxper = modeltime->getmaxper();
    summary.resize( maxper ); // object containing summaries
}

/*! \brief Destructor
* \details Deletes all subsector objects associated  with this Sector.
* \author Josh Lurz
*/
Sector::~Sector() {
    clear();
}

//! Clear member variables
void Sector::clear(){
    for( SubsectorIterator subSecIter = subsec.begin(); subSecIter != subsec.end(); subSecIter++ ) {
        delete *subSecIter;
    }
}

/*! \brief Returns Sector name
*
* \author Sonny Kim
* \return Sector name as a string
*/
const string& Sector::getName() const {
    return name;
}

/*! \brief Returns The default sector type.
* \author Steve Smith
* \return Default sector type.
*/
const string& Sector::getDefaultSectorType() {
    const static string DEFAULT_SECTOR_TYPE = "Energy";
    return DEFAULT_SECTOR_TYPE;
}

/*! \brief Return the type of the sector.
* \author Steve Smith
* \return The sector type.
*/
const string& Sector::getSectorType() const {
    return mSectorType;
}

/*! \brief Set data members from XML input
*
* \author Josh Lurz
* \param node pointer to the current node in the XML input tree
* \todo josh to add appropriate detailed comment here
*/
void Sector::XMLParse( const DOMNode* node ){
    /*! \pre make sure we were passed a valid node. */
    assert( node );

    // get the name attribute.
    name = XMLHelper<string>::getAttr( node, "name" );

    // Temporary code to warn about no longer read-in demand sector
    // perCapitaBasedString. TODO: Remove this warning.
    if( XMLHelper<bool>::getAttr( node, "perCapitaBased" ) ){
        ILogger& mainLog = ILogger::getLogger( "main_log" );
        mainLog.setLevel( ILogger::WARNING );
        mainLog << "The perCapitaBased attribute is no longer supported and will not be read."
            << " Convert the attribute to an element." << endl;
    }

    // get all child nodes.
    DOMNodeList* nodeList = node->getChildNodes();
    const Modeltime* modeltime = scenario->getModeltime();

    // loop through the child nodes.
    for( unsigned int i = 0; i < nodeList->getLength(); i++ ){
        DOMNode* curr = nodeList->item( i );
        string nodeName = XMLHelper<string>::safeTranscode( curr->getNodeName() );

        if( nodeName == "#text" ) {
            continue;
        }
        else if( nodeName == "price" ){
            // Check if the output year is the base year.
            if( XMLHelper<int>::getAttr( curr, "year" ) == modeltime->getStartYear() ){
                mBasePrice = XMLHelper<double>::getValue( curr );
            }
            else {
                // Warning?
            }
        }
        else if( nodeName == "output-unit" ){
            mOutputUnit = XMLHelper<string>::getValue( curr );
        }
        else if( nodeName == "input-unit" ){
            mInputUnit = XMLHelper<string>::getValue( curr );
        }
        else if( nodeName == "price-unit" ){
            mPriceUnit = XMLHelper<string>::getValue( curr );
        }
        else if( nodeName == "sectorType" ){
            mSectorType = XMLHelper<string>::getValue( curr );
        }
        else if( nodeName == "output" ) {
            // Check if the output year is the base year.
            if( XMLHelper<int>::getAttr( curr, "year" ) == modeltime->getStartYear() ){
                mBaseOutput = XMLHelper<double>::getValue( curr );
            }
            else {
                ILogger& mainLog = ILogger::getLogger( "main_log" );
                mainLog.setLevel( ILogger::WARNING );
                mainLog << "Output level for years other than " << modeltime->getStartYear()
                        << " are not read in."<< endl;
            }
        }
        else if ( nodeName == object_meta_info_type::getXMLNameStatic() ){
            /* Read in object meta info here into mObjectMetaInfo.  This
             * will be copied into mSectorInfo in completeInit()
             */
            object_meta_info_type metaInfo;
            if ( metaInfo.XMLParse( curr ) ){
                // Add to collection
                mObjectMetaInfo.push_back( metaInfo );
            }
        }
        else if( nodeName == MoreSectorInfo::getXMLNameStatic() ) {
            parseSingleNode( curr, moreSectorInfo, new MoreSectorInfo );
        }
        else if( nodeName == Subsector::getXMLNameStatic() ){
            parseContainerNode( curr, subsec, subSectorNameMap, new Subsector( regionName, name ) );
        }
        else if( nodeName == TranSubsector::getXMLNameStatic() ){
            parseContainerNode( curr, subsec, subSectorNameMap, new TranSubsector( regionName, name ) );
        }
        else if( nodeName == "keyword" ){
            DOMNamedNodeMap* keywordAttributes = curr->getAttributes();
            for( unsigned int attrNum = 0; attrNum < keywordAttributes->getLength(); ++attrNum ) {
                DOMNode* attrTemp = keywordAttributes->item( attrNum );
                mKeywordMap[ XMLHelper<string>::safeTranscode( attrTemp->getNodeName() ) ] =
                    XMLHelper<string>::safeTranscode( attrTemp->getNodeValue() );
            }
        }
        else if( XMLDerivedClassParse( nodeName, curr ) ){
        }
        else {
            ILogger& mainLog = ILogger::getLogger( "main_log" );
            mainLog.setLevel( ILogger::WARNING );
            mainLog << "Unrecognized text string: " << nodeName << " found while parsing " << getXMLName() << "." << endl;
        }
    }
}

/*! \brief Write object to xml output stream
*
* Method writes the contents of this object to the XML output stream.
*
* \author Josh Lurz
* \param out reference to the output stream
* \param tabs A tabs object responsible for printing the correct number of tabs.
*/
void Sector::toInputXML( ostream& aOut, Tabs* aTabs ) const {
    const Modeltime* modeltime = scenario->getModeltime();

    XMLWriteOpeningTag ( getXMLName(), aOut, aTabs, name );

    // write the xml for the class members.
    XMLWriteElementCheckDefault( mSectorType, "sectorType", aOut, aTabs, getDefaultSectorType() );
    XMLWriteElement( mOutputUnit, "output-unit", aOut, aTabs );
    XMLWriteElement( mInputUnit, "input-unit", aOut, aTabs );
    XMLWriteElement( mPriceUnit, "price-unit", aOut, aTabs );

    XMLWriteElementCheckDefault( mBasePrice, "price", aOut, aTabs, 0.0, modeltime->getper_to_yr( 0 ) );
    XMLWriteElementCheckDefault( mBaseOutput, "output", aOut, aTabs, 0.0, modeltime->getper_to_yr( 0 ) );
    if( !mKeywordMap.empty() ) {
        XMLWriteElementWithAttributes( "", "keyword", aOut, aTabs, mKeywordMap );
    }

    if ( mObjectMetaInfo.size() ) {
        for ( object_meta_info_vector_type::const_iterator metaInfoIterItem = mObjectMetaInfo.begin();
            metaInfoIterItem != mObjectMetaInfo.end(); 
            ++metaInfoIterItem ) {
                metaInfoIterItem->toInputXML( aOut, aTabs );
            }
    }

    // write out variables for derived classes
    toInputXMLDerived( aOut, aTabs );

    if( moreSectorInfo.get() ){
        moreSectorInfo->toInputXML( aOut, aTabs );
    }

    // write out the subsector objects.
    for( CSubsectorIterator k = subsec.begin(); k != subsec.end(); k++ ){
        ( *k )->toInputXML( aOut, aTabs );
    }

    // finished writing xml for the class members.
    XMLWriteClosingTag( getXMLName(), aOut, aTabs );
}

/*! \brief Write information useful for debugging to XML output stream
*
* Function writes market and other useful info to XML. Useful for debugging.
*
* \author Josh Lurz
* \param period model period
* \param out reference to the output stream
* \param aTabs A tabs object responsible for printing the correct number of tabs.
*/
void Sector::toDebugXML( const int aPeriod, ostream& aOut, Tabs* aTabs ) const {

    XMLWriteOpeningTag ( getXMLName(), aOut, aTabs, name );

    // write the xml for the class members.
    XMLWriteElement( mSectorType, "sectorType", aOut, aTabs );
    XMLWriteElement( mOutputUnit, "output-unit", aOut, aTabs );
    XMLWriteElement( mInputUnit, "input-unit", aOut, aTabs );
    XMLWriteElement( mPriceUnit, "price-unit", aOut, aTabs );

    // Write out the data in the vectors for the current period.
    XMLWriteElement( getOutput( aPeriod ), "output", aOut, aTabs );
    XMLWriteElement( getFixedOutput( aPeriod ), "fixed-output", aOut, aTabs );
    XMLWriteElement( outputsAllFixed( aPeriod ), "outputs-all-fixed", aOut, aTabs );
    XMLWriteElement( getCalOutput( aPeriod ), "cal-output", aOut, aTabs );

    if ( mObjectMetaInfo.size() ) {
        for ( object_meta_info_vector_type::const_iterator metaInfoIterItem = mObjectMetaInfo.begin();
            metaInfoIterItem != mObjectMetaInfo.end(); 
            ++metaInfoIterItem ) {
                metaInfoIterItem->toInputXML( aOut, aTabs );
            }
    }

    toDebugXMLDerived (aPeriod, aOut, aTabs);

    if( moreSectorInfo.get() ){
        moreSectorInfo->toDebugXML( aPeriod, aOut, aTabs );
    }
    // Write out the summary
    // summary[ aPeriod ].toDebugXML( aPeriod, aOut );

    // write out the subsector objects.
    for( CSubsectorIterator j = subsec.begin(); j != subsec.end(); j++ ){
        ( *j )->toDebugXML( aPeriod, aOut, aTabs );
    }

    // finished writing xml for the class members.

    XMLWriteClosingTag( getXMLName(), aOut, aTabs );
}

/*! \brief Complete the initialization
*
* This routine is only called once per model run
*
* \author Josh Lurz
* \param aRegionInfo Regional information object.
* \param aDepFinder Regional dependency finder.
* \param aLandAllocator Regional land allocator.
* \param aGlobalTechDB Global Technology database.
* \warning markets are not necessarily set when completeInit is called
*/
void Sector::completeInit( const IInfo* aRegionInfo, DependencyFinder* aDepFinder,
                           ILandAllocator* aLandAllocator, const GlobalTechnologyDatabase* aGlobalTechDB )
{
    // Allocate the sector info.
    mSectorInfo.reset( InfoFactory::constructInfo( aRegionInfo, regionName + "-" + name ) );
    // Set output and price unit of sector into sector info.
    mSectorInfo->setString( "output-unit", mOutputUnit );
    mSectorInfo->setString( "input-unit", mInputUnit );
    mSectorInfo->setString( "price-unit", mPriceUnit );

    if ( mObjectMetaInfo.size() ) {
        // Put values in mSectorInfo
        for ( object_meta_info_vector_type::const_iterator metaInfoIterItem = mObjectMetaInfo.begin(); 
            metaInfoIterItem != mObjectMetaInfo.end();
            ++metaInfoIterItem ) {
                mSectorInfo->setDouble( (*metaInfoIterItem).getName(), (*metaInfoIterItem).getValue() );
            }
    }

    // Complete the subsector initializations.
    for( vector<Subsector*>::iterator subSecIter = subsec.begin(); subSecIter != subsec.end(); subSecIter++ ) {
        ( *subSecIter )->completeInit( mSectorInfo.get(), aDepFinder, aLandAllocator, aGlobalTechDB );
    }
}

/*! \brief Perform any initializations needed for each period.
*
* Any initializations or calculations that only need to be done once per period
* (instead of every iteration) should be placed in this function.
*
* \author Steve Smith
* \param aPeriod Model period
*/
void Sector::initCalc( NationalAccount* aNationalAccount,
                      const Demographic* aDemographics,
                      const int aPeriod )
{
    // normalizeShareWeights must be called before subsector initializations
    normalizeShareWeights( aPeriod );

    Marketplace* marketplace = scenario->getMarketplace();

    // do any sub-Sector initializations
    for ( unsigned int i = 0; i < subsec.size(); ++i ){
        subsec[ i ]->initCalc( aNationalAccount, aDemographics, moreSectorInfo.get(), aPeriod );
    }
}

/*! \brief Warns if user may have missed entering a calibration value.
*
* Prints a warning if a sector containts mostly calibration values but not every potential output has
* a calibration value entered. In this case the sector will likely not correctly calibrate.
* Print warning in this situation.
*
* \author Steve Smith
* \param period Model period
*/
void Sector::checkForCalibrationConsistancy( const int aPeriod ) {

    // Don't bother checking for base period.
    if ( aPeriod == 0 ) {
        return;
    }

    // Get fuel consumption map for sector.
    InputFinder inputFinder;
    accept( &inputFinder, aPeriod );
    const list<string>& inputsUsed = inputFinder.getInputs();

    double totalCalOutputs = 0;
    typedef list<string>::const_iterator CI;
    stack<string> adjustableInputs;

    for( CI currInput = inputsUsed.begin(); currInput != inputsUsed.end(); ++currInput ){
        // If inputs are all fixed for this fuel.
        // For each input, this loops thru all subsectors and all technologies.
        // If subsector share weight is 0, inputsAllFixed is true.
        if ( inputsAllFixed( aPeriod, *currInput ) ) {
            // This call loops thru all subsectors and technologies again.
            //TODO: Double accounting occurs when subsector share weights are null
            // and other subsectors have fixed outputs.
            totalCalOutputs += getCalAndFixedOutputs( aPeriod, *currInput );
            ILogger& calibrationLog = ILogger::getLogger( "calibration_log" );
            calibrationLog.setLevel( ILogger::DEBUG );
            calibrationLog  << "Input " <<  *currInput << " in region " << regionName
                << " has calibrated output of " << totalCalOutputs << endl;

        }
        // else have an input that can be adjusted
        else {
            adjustableInputs.push( *currInput );
        }
    }
    // Note -- code before this point copied from RegionMiniCAM::setImpliedCalInputs().
    // Is done here so that check is only done once and more detailed information can be printed.

    // If this sector has caloutputs but is not completely calibrated this could be a user mistake, so warn about this
    if ( ( ( totalCalOutputs - getFixedOutput( aPeriod ) ) > util::getSmallNumber() ) && ( adjustableInputs.size() > 0 ) ) {
        ILogger& calibrationLog = ILogger::getLogger( "calibration_log" );
        calibrationLog.setLevel( ILogger::DEBUG );
        calibrationLog  << "Sector " <<  getName() << " in region " << regionName
            << " has calibrated output of " << totalCalOutputs
            << " but " << adjustableInputs.size() << " uncalibrated inputs: " << endl;

        stack<string> adjustableInputsCopy = adjustableInputs;

        calibrationLog << "    "; // indent
        while( !adjustableInputs.empty() ){
            calibrationLog << adjustableInputs.top() << ", ";
            adjustableInputs.pop();
        }
        calibrationLog << endl;

        calibrationLog << "    Suspect sub-sectors: ";
        for ( unsigned int i = 0; i < subsec.size(); ++i ){
            if ( !(subsec[ i ]->inputsAllFixed( aPeriod, "allInputs" ) ) ) {
                calibrationLog << subsec[ i ]->getName() << ", ";
            }
        }
        calibrationLog << endl;
        calibrationLog << "Fixed outputs = " << getFixedOutput( aPeriod ) << endl;

        calibrationLog<< "All Inputs: ";
        for( CI currInput = inputsUsed.begin(); currInput != inputsUsed.end(); ++currInput ){
            calibrationLog << *currInput << ", ";
        }
        calibrationLog << endl;
        ILogger& mainLog = ILogger::getLogger( "main_log" );
        mainLog.setLevel( ILogger::ERROR );
        mainLog << "Potential calibration data issue with sector " << name 
            << " in region " << regionName << "." 
            << " See calibration_log for details."<< endl;
    }
}
 
/*! \brief Scales subsector share weights so that they equal number of subsectors.
*
* This is needed so that 1) share weights can be easily interpreted (> 1 means favored) and so that
* future share weights can be consistently applied relative to calibrated years.
*
* \author Steve Smith
* \param period Model period
* \warning This must be done before subsector inits so that share weights are scaled before they are interpolated
*/
void Sector::normalizeShareWeights( const int period ) {

    // If this sector was completely calibrated, or otherwise fixed, then scale shareweights to equal number of subsectors
    static const bool calActive = Configuration::getInstance()->getBool( "CalibrationActive" );
    if ( period > 0 && calActive ) {
        if ( inputsAllFixed( period - 1, "allInputs" ) && ( getCalOutput ( period - 1) > 0 ) ) {

            double shareWeightTotal = 0;
            int numberNonzeroSubSectors = 0;
            for ( unsigned int i = 0; i < subsec.size(); ++i ){
                double subsectShareWeight = subsec[ i ]->getShareWeight( period - 1 );
                shareWeightTotal += subsectShareWeight;
                if ( subsectShareWeight > 0 ) {
                    numberNonzeroSubSectors += 1;
                }
            }

            if ( shareWeightTotal < util::getTinyNumber() ) {
                ILogger& mainLog = ILogger::getLogger( "main_log" );
                mainLog.setLevel( ILogger::ERROR );
                mainLog << "Shareweights sum to zero for sector " << name << "." << endl;
            }
            else {
                for ( int unsigned i= 0; i< subsec.size(); i++ ) {
                    subsec[ i ]->scaleShareWeight( numberNonzeroSubSectors / shareWeightTotal, period - 1 );
                }
                ILogger& calibrationLog = ILogger::getLogger( "calibration_log" );
                calibrationLog.setLevel( ILogger::DEBUG );
                calibrationLog << "Shareweights normalized for sector " << name << " in region " << regionName << endl;
            }
        }
    }
}

/*! \brief Test to see if calibration worked for this sector
*
* Compares the sum of calibrated + fixed values to output of sector.
* Will optionally print warning to the screen (and eventually log file).
*
* If all outputs are not calibrated then this does not check for consistency.
*
* \author Steve Smith
* \param period Model period
* \param calAccuracy Accuracy (fraction) to check if calibrations are within.
* \param printWarnings if true prints a warning
* \return Boolean true if calibration is ok.
*/
bool Sector::isAllCalibrated( const int period, double calAccuracy, const bool printWarnings ) const {
    bool isAllCalibrated = true;
    // Check if each subsector is calibrated.
    for( unsigned int i = 0; i < subsec.size(); ++i ){
        isAllCalibrated &= subsec[ i ]->isAllCalibrated( period, calAccuracy, printWarnings );
    }
    return isAllCalibrated;
}

/*!
 * \brief Calculate technology costs for the Sector.
 * \param aPeriod Period.
 * \todo Move to supply sector and make private once demand and supply sectors
 *       are separate.
 */
void Sector::calcCosts( const int aPeriod ){
    // Instruct all subsectors to calculate their costs. This must be done
    // before prices can be calculated.
    for( unsigned int i = 0; i < subsec.size(); ++i ){
        subsec[ i ]->calcCost( aPeriod );
    }
}

/*! \brief Calculate the shares for the subsectors.
* \details This routine calls subsector::calcShare for each subsector, which
*          calculated an unnormalized share, and then calls normShare to
*          normalize the shares for each subsector. Fixed subsectors are ignored
*          here as they do not have a share of the new investment.
* \param aGDP Regional GDP container.
* \param aPeriod Model period.
* \return A vector of normalized shares, one per subsector, ordered by subsector.
*/
const vector<double> Sector::calcSubsectorShares( const GDP* aGDP, const int aPeriod ) const {
    // Calculate unnormalized shares.
    vector<double> subsecShares( subsec.size() );
    for( unsigned int i = 0; i < subsec.size(); ++i ){
        subsecShares[ i ] = subsec[ i ]->calcShare( aPeriod, aGDP );
    }

    // Normalize the shares.
    double shareSum = SectorUtils::normalizeShares( subsecShares );
    if( !util::isEqual( shareSum, 1.0 ) && !outputsAllFixed( aPeriod ) ){
        ILogger& mainLog = ILogger::getLogger( "main_log" );
        mainLog.setLevel( ILogger::DEBUG );
        mainLog << "Shares for sector " << name << " in region " << regionName
            << " did not normalize correctly. Sum is " << shareSum << "." << endl;
    }
    /*! \post There is one share per subsector. */
    assert( subsecShares.size() == subsec.size() );
    return subsecShares;
}

/*! \brief Calculate and return weighted average price of subsectors.
* \param period Model period
* \return The weighted sector price.
* \author Sonny Kim, Josh Lurz, James Blackwood
* \param period Model period
* \return Weighted sector price.
*/
double Sector::getPrice( const GDP* aGDP, const int aPeriod ) const {
    const vector<double> subsecShares = calcSubsectorShares( aGDP, aPeriod );
    double sectorPrice = 0;
    double sumSubsecShares = 0;
    for ( unsigned int i = 0; i < subsec.size(); ++i ){
        // Subsectors with no share cannot affect price. The getPrice function
        // is constant so skipping it will not avoid any side effects. What?
        if( subsecShares[ i ] > util::getSmallNumber() ){
            sumSubsecShares += subsecShares[ i ];
            double currPrice = subsec[ i ]->getPrice( aGDP, aPeriod );
            // Check for negative prices returned by fixed investment.
            if( currPrice >= util::getSmallNumber() ){
                sectorPrice += subsecShares[ i ] * currPrice;
            }
            // We want to allow regional biomass to have a negative price. These are
            // not solved markets.
            else if ( ( name == "regional biomass" || name == "delivered biomass" ) ) {
                sectorPrice += subsecShares[ i ] * currPrice;
            }
        }
    }

    // For sectors with non-zero subsector shares, check if the overall sector price is zero.
    // Zero sector price would cause infinite demand.
    if( (sumSubsecShares > util::getSmallNumber()) 
             && (sectorPrice < util::getSmallNumber()) ){
        // Allow sector price to be negative
        if ( ( name != "regional biomass" && name != "delivered biomass" ) ){
            ILogger& mainLog = ILogger::getLogger( "main_log" );
            mainLog.setLevel( ILogger::DEBUG );
            mainLog << "Zero price for sector " << name << " in region " << regionName
                    << " Resetting to last period's price." << endl;
            sectorPrice = aPeriod > 0 ? getPrice( aGDP, aPeriod - 1 ) : 1;
        }
    }
    return sectorPrice;
}

/*! \brief Returns true if all sub-Sector outputs are fixed or calibrated.
*
* Routine loops through all the subsectors in the current Sector. If output is
* calibrated, assigned a fixed output, or set to zero (because share weight is
* zero) then true is returned. If all ouptput is not fixed, then the Sector has
* at least some capacity to respond to a change in prices.
*
* \author Steve Smith
* \param period Model period
* \return Boolean that is true if entire Sector is calibrated or has fixed
*         output
*/
bool Sector::outputsAllFixed( const int period ) const {
    assert( period >= 0 );
    for ( unsigned int i = 0; i < subsec.size(); ++i ){
        if ( !( subsec[ i ]->allOutputFixed( period ) ) ) {
            return false;
        }
    }
    return true;
}

/*!
 * \brief Calculate the total amount of fixed output in the Sector.
 * \details Fixed output is defined as infra-marginal output. This means that
 *          the production of this output is below the margin, and so does not
 *          affect the marginal cost of producing the Sector's output. Fixed
 *          output may be the output of vintages, or may be fixed investment
 *          that is determined by exogenous, non-cost based, factors. Fixed
 *          output should never be used to specify an investment or output
 *          pathway for a good that should be competitively determined. For
 *          example, investment in hydro-electricity is input as fixed output,
 *          because the investment is determined not be marginal cost but by
 *          government decisions. Since fixed output is infra-marginal, it is
 *          not included in the cost calculation. The total fixed output is
 *          removed from the desired output of the sector before the output is
 *          distributed to variable output technologies(which are on the
 *          margin).
 * \note Currently the price of a Sector with all fixed output is undefined. If
 *       the model encounters the condition, it will set variable output to zero
 *       and scale fixed output to equal total output. The price will be set as
 *       the previous period's price. This is not generally an issue in
 *       equilibrium, as the socio-economic scenarios used have increasing
 *       output, and depreciation of capital causes new capital to be required.
 *       If this became an issue, there are two potential solutions: A fraction
 *       of marginal output could be forced into the Sector, which would assist
 *       the model to solve but not be economically consistent. The economically
 *       consistent solution would be to back down the supply schedule and
 *       shutdown the marginal fixed output producer.
 *
 * \author Steve Smith, Josh Lurz
 * \param period Model period
 * \return Total fixed output.
 */
double Sector::getFixedOutput( const int period ) const {
    double totalfixedOutput = 0;
    for ( unsigned int i = 0; i < subsec.size(); ++i ){
        totalfixedOutput += subsec[ i ]->getFixedOutput( period );
    }
    return totalfixedOutput;
}

/*! \brief Return subsector total calibrated outputs.
*
* Returns the total calibrated outputs from all subsectors and technologies.
* Note that any calibrated input values are converted to outputs and are included.
*
* This returns only calibrated outputs, not values otherwise fixed (as fixed or zero share weights)
*
* \author Steve Smith
* \param period Model period
* \return total calibrated outputs
*/
double Sector::getCalOutput( const int period  ) const {
    double totalCalOutput = 0;
    for ( unsigned int i = 0; i < subsec.size(); ++i ){
        totalCalOutput += subsec[ i ]->getTotalCalOutputs( period );
    }
    return totalCalOutput;
}

/*! \brief Return subsector total fixed or calibrated inputs.
*
* Returns the total fixed inputs from all subsectors and technologies.
* Note that any calibrated output values are converted to inputs and are included.
*
* \author Steve Smith
* \param period Model period
* \param goodName market good to return inputs for. If equal to the value "allInputs" then returns all inputs.
* \return total fixed inputs
*/
double Sector::getCalAndFixedOutputs( const int period, const string& goodName ) const {
    double sumCalOutputValues = 0;
    for ( unsigned int i = 0; i < subsec.size(); ++i ){
        sumCalOutputValues += subsec[ i ]->getCalAndFixedOutputs( period, goodName );
    }
    return sumCalOutputValues;
}

/*! \brief Sets the input value needed to produce the required output to the marketplace
*
* \author Steve Smith
* \param period Model period
* \param goodName market good to determine the inputs for.
* \param requiredOutput Amount of output to produce
*/
void Sector::setImpliedFixedInput( const int period, const string& goodName,
                                   const double requiredOutput )
{
    bool inputWasChanged = false;
    for ( unsigned int i = 0; i < subsec.size(); ++i ){
        if ( !inputWasChanged ) {
            inputWasChanged = subsec[ i ]->setImpliedFixedInput( period, goodName,
                                                                 requiredOutput );
        }
        else {
            if( subsec[ i ]->setImpliedFixedInput( period, goodName, requiredOutput ) ){
                ILogger& mainLog = ILogger::getLogger( "main_log" );
                mainLog.setLevel( ILogger::NOTICE );
                mainLog << "caldemands for more than one subsector were changed for good "
                        << goodName << " in sector " << name << " in region "
                        << regionName << endl; 
            }
        }
    }
}

/*! \brief Returns true if all subsector inputs for the the specified good are fixed.
*
* Fixed inputs can be by either fixedCapacity, calibration, or zero share.
*
* \author Steve Smith
* \param period Model period
* \param goodName market good to return inputs for. If equal to the value "allInputs" then returns all inputs.
* \return total calibrated inputs
*/
bool Sector::inputsAllFixed( const int period, const string& goodName ) const {
    for ( unsigned int i = 0; i < subsec.size(); ++i ){
        if ( !(subsec[ i ]->inputsAllFixed( period, goodName ) ) ){
            return false;
        }
    }
    return true;
}

/*! \brief Scales calibrated values for the specified good.
*
* \author Steve Smith
* \param period Model period
* \param goodName market good to return inputs for
* \param scaleValue multipliciative scaler for calibrated values
* \return total calibrated inputs
*/
void Sector::scaleCalibratedValues( const int period, const string& goodName, const double scaleValue ) {
    for ( unsigned int i = 0; i < subsec.size(); ++i ){
        subsec[ i ]->scaleCalibratedValues( period, goodName, scaleValue );
    }
}

/*! \brief Calibrate Sector output.
*
* This performs supply Sector technology and sub-Sector output/input calibration.
Determines total amount of calibrated and fixed output and passes that down to the subsectors.

* Note that this routine only performs subsector and technology-level calibration. Total final energy calibration is done by Region::calibrateTFE and GDP calibration is set up in Region::calibrateRegion.
*
* \author Steve Smith
* \param period Model period
*/
void Sector::calibrateSector( const GDP* aGDP, const int aPeriod ) {
    double marketDemand = scenario->getMarketplace()->getDemand( name, regionName, aPeriod );
    // Calculate the demand for new investment.
    double variableDemand = max( marketDemand - getFixedOutput( aPeriod ), 0.0 );

    const vector<double> subsecShares = calcSubsectorShares( aGDP, aPeriod );
    bool hasOneSubsector = ( subsec.size() == 1 );
    for ( unsigned int i = 0; i < subsec.size(); ++i ){
        double subsectorDemand = subsecShares[ i ] * variableDemand;
        subsec[ i ]->adjustForCalibration( subsectorDemand, hasOneSubsector, aGDP, aPeriod );
    }
}


/*! \brief Calculate GHG emissions for each Sector from subsectors.
*
* Calculates emissions for subsectors and technologies, then updates emissions maps for emissions by gas and emissions by fuel & gas.
*
* Note that at present (10/03), emissions only occur at technology level.
*
* \author Sonny Kim
* \param period Model period
*/
void Sector::emission( const int period ) {
    summary[ period ].clearemiss(); // clear emissions map
    summary[ period ].clearemfuelmap(); // clear emissions fuel map
    for( unsigned int i = 0; i < subsec.size(); ++i ){
        subsec[ i ]->emission( period );
        summary[ period ].updateemiss( subsec[ i ]->getemission( period )); // by gas
        summary[ period ].updateemfuelmap( subsec[ i ]->getemfuelmap( period )); // by fuel and gas
    }
}

//! Write Sector output to database.
void Sector::csvOutputFile( const GDP* aGDP,
                            const IndirectEmissionsCalculator* aIndirectEmissCalc ) const {
    // function protocol
    void fileoutput3( string var1name,string var2name,string var3name,
        string var4name,string var5name,string uname,vector<double> dout);

    // function arguments are variable name, double array, db name, table name
    // the function writes all years
    // total Sector output
    const Modeltime* modeltime = scenario->getModeltime();
    const int maxper = modeltime->getmaxper();
    vector<double> temp(maxper);
    // sector output or production
    for( int per = 0; per < maxper; ++per ){
        temp[ per ] = getOutput( per );
    }
    fileoutput3( regionName, name, " ", " ", "production", mOutputUnit, temp );

    // Sector price
    for( int per = 0; per < maxper; ++per ){
        temp[ per ] = getPrice( aGDP, per );
    }
    fileoutput3( regionName, name, " ", " ", "price", mPriceUnit, temp);

    // do for all subsectors in the Sector
    for( unsigned int i = 0; i < subsec.size(); ++i ){
        // output or demand for each technology
        subsec[ i ]->csvOutputFile( aGDP, aIndirectEmissCalc );
    }
}

/*! \brief Return fuel consumption map for this Sector
*
* \author Sonny Kim
* \param period Model period
* \todo Input change name of this and other methods here to proper capitilization
* \return fuel consumption map
*/
map<string, double> Sector::getfuelcons( const int period ) const {
    return summary[ period ].getfuelcons();
}

//!  Get the second fuel consumption map in summary object.
/*! \brief Return fuel consumption for the specifed fuel
*
* \author Sonny Kim
* \param period Model period
* \param fuelName name of fuel
* \return fuel consumption
*/
double Sector::getConsByFuel( const int period, const std::string& fuelName ) const {
    return summary[ period ].get_fmap_second( fuelName );
}

/*! \brief Return the ghg emissions map for this Sector
*
* \author Sonny Kim
* \param period Model period
* \return GHG emissions map
*/
map<string, double> Sector::getemission( const int period ) const {
    return summary[ period ].getemission();
}

/*! \brief Return ghg emissions map in summary object
*
* This map is used to calculate the emissions coefficient for this Sector (and fuel?) in region
*
* \author Sonny Kim
* \param period Model period
* \return GHG emissions map
*/
map<string, double> Sector::getemfuelmap( const int period ) const {
    return summary[ period ].getemfuelmap();
}

/*! \brief update summaries for reporting
*
*  Updates summary information for the Sector and all subsectors.
*
* \author Sonny Kim
* \param period Model period
* \return GHG emissions map
*/
void Sector::updateSummary( const list<string>& aPrimaryFuelList, const int period ) {
    // clears Sector fuel consumption map
    summary[ period ].clearfuelcons();

    for( unsigned int i = 0; i < subsec.size(); ++i ){
        // call update summary for subsector
        subsec[ i ]->updateSummary( aPrimaryFuelList, period );
        // sum subsector fuel consumption for Sector fuel consumption
        summary[ period ].updatefuelcons( aPrimaryFuelList, subsec[ i ]->getfuelcons( period ));
    }
}

/*! \brief Initialize the marketplaces in the base year to get initial demands from each technology in subsector
*
* \author Pralit Patel
* \param period The period is usually the base period
*/
void Sector::updateMarketplace( const int period ) {
    for( unsigned int i = 0; i < subsec.size(); i++ ) {
        subsec[ i ]->updateMarketplace( period );
    }
}

/*! \brief Function to finalize objects after a period is solved.
* \details This function is used to calculate and store variables which are only needed after the current
* period is complete.
* \param aPeriod The period to finalize.
* \todo Finish this function.
* \author Josh Lurz
*/
void Sector::postCalc( const int aPeriod ){
    // Finalize sectors.
    for( SubsectorIterator subsector = subsec.begin(); subsector != subsec.end(); ++subsector ){
        (*subsector)->postCalc( aPeriod );
    }
}

/*! \brief For outputting SGM data to a flat csv File
*
* \author Pralit Patel
* \param period The period which we are outputting for
*/
void Sector::csvSGMOutputFile( ostream& aFile, const int period ) const {

    // when csvSGMOutputFile() is called, a new sector report is created, updated and printed
    // this function writes a sector report for each sector
    auto_ptr<IVisitor> sectorReport( new SectorReport( aFile ) );
    accept( sectorReport.get(), period );
    sectorReport->finish();
    for( unsigned int i = 0; i < subsec.size(); i++ ) {
        subsec[ i ]->csvSGMOutputFile( aFile, period );
    }
}

void Sector::accept( IVisitor* aVisitor, const int aPeriod ) const {
    aVisitor->startVisitSector( this, aPeriod );
    for( unsigned int i = 0; i < subsec.size(); i++ ) {
        subsec[ i ]->accept( aVisitor, aPeriod );
    }
    aVisitor->endVisitSector( this, aPeriod );
}