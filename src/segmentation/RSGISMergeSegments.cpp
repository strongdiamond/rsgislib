/*
 *  RSGISMergeSegments.cpp
 *  RSGIS_LIB
 *
 *  Created by Pete Bunting on 01/04/2015.
 *  Copyright 2015 RSGISLib.
 *
 *  RSGISLib is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RSGISLib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RSGISLib.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "segmentation/RSGISMergeSegments.h"


namespace rsgis{namespace segment{
    
    RSGISMergeSegments::RSGISMergeSegments()
    {
        
    }
    
    void RSGISMergeSegments::mergeSelectedClumps(GDALDataset *clumpsImage, GDALDataset *valsImageDS, std::string clumps2MergeCol, std::string noDataClumpsCol)throw(rsgis::img::RSGISImageCalcException)
    {
        try
        {
            std::cout << "Populate Neighbours\n";
            rastergis::RSGISFindClumpNeighbours findNeighboursObj;
            findNeighboursObj.findNeighboursKEAImageCalc(clumpsImage, 1);
            std::cout << "Populated Neighbours\n";
            
            std::cout << "Calculate Stats\n";
            int numSpecBands = valsImageDS->GetRasterCount();
            
            utils::RSGISTextUtils textUtils;
            
            std::vector<rsgis::rastergis::RSGISBandAttStats*> *bandStats = new std::vector<rsgis::rastergis::RSGISBandAttStats*>();
            bandStats->reserve(numSpecBands);
            
            std::vector<std::string> colNames;
            colNames.reserve(numSpecBands);
            
            rsgis::rastergis::RSGISBandAttStats *bandStat = NULL;
            for(int i = 0; i < numSpecBands; ++i)
            {
                std::string bandName = "Band" + textUtils.int32bittostring(i+1);
                std::string bandNameMean = "Mean"+bandName;
                std::string bandNameSum = "Sum"+bandName;
                
                bandStat = new rsgis::rastergis::RSGISBandAttStats();
                bandStat->band = i+1;
                bandStat->calcMin = false;
                bandStat->minField = "";
                bandStat->calcMax = false;
                bandStat->maxField = "";
                bandStat->calcMean = true;
                bandStat->meanField = bandNameMean;
                bandStat->calcStdDev = false;
                bandStat->stdDevField = "";
                bandStat->calcSum = true;
                bandStat->sumField = bandNameSum;
                
                bandStats->push_back(bandStat);
                colNames.push_back(bandName);
            }
            
            rsgis::rastergis::RSGISPopRATWithStats clumpStats;
            clumpStats.populateRATWithBasicStats(clumpsImage, valsImageDS, bandStats, 1);
            
            delete bandStats;
            std::cout << "Calculated Stats\n";
            
            rastergis::RSGISRasterAttUtils attUtils;
            
            GDALRasterBand *clumpBand = clumpsImage->GetRasterBand(1);
            GDALRasterAttributeTable *rat = clumpBand->GetDefaultRAT();
            size_t numRows = rat->GetRowCount();
            std::cout << "Number of clumps is " << numRows << "\n";
            
            std::cout << "Read in neighbours\n";
            std::vector<std::vector<size_t>* > *neighbours = attUtils.getRATNeighbours(clumpsImage, 1);
            
            if(numRows != neighbours->size())
            {
                for(std::vector<std::vector<size_t>* >::iterator iterNeigh = neighbours->begin(); iterNeigh != neighbours->end(); ++iterNeigh)
                {
                    delete *iterNeigh;
                }
                delete neighbours;
                
                throw rsgis::RSGISAttributeTableException("RAT size is different to the number of neighbours retrieved.");
            }
            
            size_t tmpNumRows = 0;
            int *selectCol = attUtils.readIntColumn(rat, clumps2MergeCol, &tmpNumRows);
            int *noDataCol = attUtils.readIntColumn(rat, noDataClumpsCol, &tmpNumRows);
            std::cout << "Read input column\n";
            
            double *numPxls = new double[numRows];
            
            double **meanVals = new double*[numSpecBands];
            double **sumVals = new double*[numSpecBands];
            
            for(int i = 0; i < numSpecBands; ++i)
            {
                tmpNumRows = 0;
                meanVals[i] = attUtils.readDoubleColumn(rat, "Mean"+colNames.at(i), &tmpNumRows);
                if(tmpNumRows != numRows)
                {
                    throw rsgis::img::RSGISImageCalcException("Number of rows was incorrect. (Mean)");
                }
                tmpNumRows = 0;
                sumVals[i] = attUtils.readDoubleColumn(rat, "Sum"+colNames.at(i), &tmpNumRows);
                if(tmpNumRows != numRows)
                {
                    throw rsgis::img::RSGISImageCalcException("Number of rows was incorrect. (Sum)");
                }
            }
            
            
            
            std::vector<rsgisClumpInfo*> clumps;
            for(size_t i = 0; i < numRows; ++i)
            {
                rsgisClumpInfo *clump = new rsgisClumpInfo();
                clump->clumpID = i;
                if(noDataCol[i] == 1)
                {
                    clump->clumpID = 0;
                }
                clump->origClumpIDs.push_back(i);
                clump->merge = false;
                clump->removed = false;
                clump->mergeTo = NULL;
                clump->meanVals = new double[numSpecBands];
                clump->sumVals = new double[numSpecBands];
                clump->numVals = numSpecBands;
                for(size_t j = 0; j < numSpecBands; ++j)
                {
                    clump->meanVals[j] = meanVals[j][i];
                    clump->sumVals[j] = sumVals[j][i];
                }
                clump->numPxls = sumVals[0][i] / meanVals[0][i];
                clump->neighbours.clear();
                clump->selected = selectCol[i];
                clump->noDataRegion = noDataCol[i];
                clumps.push_back(clump);
            }
            for(size_t i = 0; i < numRows; ++i)
            {
                for(size_t j = 0; j < neighbours->at(i)->size(); ++j)
                {
                    clumps.at(i)->neighbours.push_back(clumps.at(neighbours->at(i)->at(j)));
                }
            }
            
            
            double val = 0.0;
            bool process = true;
            bool first = true;
            rsgisClumpInfo *minClump = NULL;
            double minVal = 0.0;
            bool found = false;
            unsigned int processIter = 1;
            rsgisClumpInfo *cClump;
            rsgisClumpInfo *mClump;
            while(process)
            {
                std::cout << "Processing Iteration " << processIter++ << std::endl;
                process = false;
                for(size_t i = 0; i < clumps.size(); ++i)
                {
                    cClump = clumps.at(i);
                    if((cClump->selected == 1) & (!cClump->removed))
                    {
                        //std::cout << "\t Process clump " << cClump->clumpID;
                        first = true;

                        for(std::list<rsgisClumpInfo*>::iterator iterNeigh = cClump->neighbours.begin(); iterNeigh != cClump->neighbours.end(); ++iterNeigh)
                        {
                            if(((*iterNeigh)->selected != 1) & (!(*iterNeigh)->removed) & ((*iterNeigh)->noDataRegion != 1))
                            {
                                val = this->calcDist(cClump->meanVals, (*iterNeigh)->meanVals, numSpecBands);
                                if(first)
                                {
                                    minClump = (*iterNeigh);
                                    minVal = val;
                                    first = false;
                                }
                                else if(val < minVal)
                                {
                                    minClump = (*iterNeigh);
                                    minVal = val;
                                }
                            }
                        }
                        
                        if(!first)
                        {
                            cClump->merge = true;
                            cClump->mergeTo = minClump;
                            //std::cout << " is merged with " << minClump->clumpID;
                            process = true;
                        }
                        //std::cout << std::endl;
                    }
                }
                
                if(process)
                {
                    for(size_t i = 0; i < clumps.size(); ++i)
                    {
                        cClump = clumps.at(i);
                        if(cClump->merge)
                        {
                            mClump = cClump->mergeTo;
                            //std::cout << "Updating " << cClump->clumpID << " to " << mClump->clumpID << std::endl;
                            cClump->removed = true;
                            mClump->origClumpIDs.push_back(cClump->clumpID);
                            
                            mClump->numPxls += cClump->numPxls;
                            for(int n = 0; n < numSpecBands; ++n)
                            {
                                mClump->sumVals[n] += cClump->sumVals[n];
                                mClump->meanVals[n] = mClump->sumVals[n] / mClump->numPxls;
                            }
                            
                            for(std::list<rsgisClumpInfo*>::iterator iterNeigh = mClump->neighbours.begin(); iterNeigh != mClump->neighbours.end(); ++iterNeigh)
                            {
                                if((*iterNeigh)->clumpID == cClump->clumpID)
                                {
                                    mClump->neighbours.erase(iterNeigh);
                                    break;
                                }
                            }
                            
                            for(std::list<rsgisClumpInfo*>::iterator iterNeighC = cClump->neighbours.begin(); iterNeighC != cClump->neighbours.end(); ++iterNeighC)
                            {
                                if((*iterNeighC)->clumpID != mClump->clumpID)
                                {
                                    found = false;
                                    for(std::list<rsgisClumpInfo*>::iterator iterNeighM = mClump->neighbours.begin(); iterNeighM != mClump->neighbours.end(); ++iterNeighM)
                                    {
                                        if((*iterNeighM)->clumpID == (*iterNeighC)->clumpID)
                                        {
                                            found = true;
                                            break;
                                        }
                                    }
                                    if(!found)
                                    {
                                        mClump->neighbours.push_back(*iterNeighC);
                                        found = false;
                                        for(std::list<rsgisClumpInfo*>::iterator iterNeighSec = (*iterNeighC)->neighbours.begin(); iterNeighSec != (*iterNeighC)->neighbours.end(); ++iterNeighSec)
                                        {
                                            if((*iterNeighSec)->clumpID == mClump->clumpID)
                                            {
                                                found = true;
                                                break;
                                            }
                                        }
                                        if(!found)
                                        {
                                            (*iterNeighC)->neighbours.push_back(mClump);
                                        }
                                    }
                                }
                            }
                            cClump->neighbours.clear();
                        }
                    }
                }
            }
            std::cout << "Completed Iterations\n";
            
            
            int *clumpIDUp = new int[numRows];
            for(size_t i = 0; i < clumps.size(); ++i)
            {
                cClump = clumps.at(i);
                if(!cClump->removed)
                {
                    for(std::vector<unsigned int>::iterator iterClumpIDs = cClump->origClumpIDs.begin(); iterClumpIDs != cClump->origClumpIDs.end(); ++iterClumpIDs)
                    {
                        clumpIDUp[*iterClumpIDs] = cClump->clumpID;
                    }
                }
            }
            attUtils.writeIntColumn(rat, "OutClumpIDs", clumpIDUp, numRows);
            
            for(size_t i = 0; i < clumps.size(); ++i)
            {
                cClump = clumps.at(i);
                cClump->neighbours.clear();
                delete cClump->meanVals;
                delete cClump->sumVals;
                delete cClump;
            }
            clumps.clear();
            
            
            for(int i = 0; i < numSpecBands; ++i)
            {
                delete[] meanVals[i];
                delete[] sumVals[i];
            }
            delete[] meanVals;
            delete[] sumVals;
            delete[] numPxls;
            delete[] selectCol;
            delete[] noDataCol;
            delete[] clumpIDUp;
            
        }
        catch(rsgis::img::RSGISImageCalcException &e)
        {
            throw e;
        }
        catch (rsgis::RSGISException &e)
        {
            throw rsgis::img::RSGISImageCalcException(e.what());
        }
        catch (std::exception &e)
        {
            throw rsgis::img::RSGISImageCalcException(e.what());
        }
    }
    /*
    void RSGISMergeSegments::mergeSelectedClumps(GDALDataset *clumpsImage, GDALDataset *valsImageDS, std::string clumps2MergeCol)throw(rsgis::img::RSGISImageCalcException)
    {
        try
        {
            std::cout << "Populate Neighbours\n";
            rastergis::RSGISFindClumpNeighbours findNeighboursObj;
            findNeighboursObj.findNeighboursKEAImageCalc(clumpsImage, 1);
            std::cout << "Populated Neighbours\n";
            
            std::cout << "Calculate Stats\n";
            int numSpecBands = valsImageDS->GetRasterCount();
            
            utils::RSGISTextUtils textUtils;
            
            std::vector<rsgis::rastergis::RSGISBandAttStats*> *bandStats = new std::vector<rsgis::rastergis::RSGISBandAttStats*>();
            bandStats->reserve(numSpecBands);
            
            std::vector<std::string> colNames;
            colNames.reserve(numSpecBands);
            
            rsgis::rastergis::RSGISBandAttStats *bandStat = NULL;
            for(int i = 0; i < numSpecBands; ++i)
            {
                std::string bandName = "Band" + textUtils.int32bittostring(i+1);
                std::string bandNameMean = "Mean"+bandName;
                std::string bandNameSum = "Sum"+bandName;
                
                bandStat = new rsgis::rastergis::RSGISBandAttStats();
                bandStat->band = i+1;
                bandStat->calcMin = false;
                bandStat->minField = "";
                bandStat->calcMax = false;
                bandStat->maxField = "";
                bandStat->calcMean = true;
                bandStat->meanField = bandNameMean;
                bandStat->calcStdDev = false;
                bandStat->stdDevField = "";
                bandStat->calcSum = true;
                bandStat->sumField = bandNameSum;
                
                bandStats->push_back(bandStat);
                colNames.push_back(bandName);
            }
            
            rsgis::rastergis::RSGISPopRATWithStats clumpStats;
            clumpStats.populateRATWithBasicStats(clumpsImage, valsImageDS, bandStats, 1);
            
            delete bandStats;
            std::cout << "Calculated Stats\n";
            
            rastergis::RSGISRasterAttUtils attUtils;
            
            GDALRasterBand *clumpBand = clumpsImage->GetRasterBand(1);
            GDALRasterAttributeTable *rat = clumpBand->GetDefaultRAT();
            size_t numRows = rat->GetRowCount();
            std::cout << "Number of clumps is " << numRows << "\n";
            
            std::cout << "Read in neighbours\n";
            std::vector<std::vector<size_t>* > *neighbours = attUtils.getRATNeighbours(clumpsImage, 1);
            
            if(numRows != neighbours->size())
            {
                for(std::vector<std::vector<size_t>* >::iterator iterNeigh = neighbours->begin(); iterNeigh != neighbours->end(); ++iterNeigh)
                {
                    delete *iterNeigh;
                }
                delete neighbours;
                
                throw rsgis::RSGISAttributeTableException("RAT size is different to the number of neighbours retrieved.");
            }
            
            size_t tmpNumRows = 0;
            int *selectCol = attUtils.readIntColumn(rat, clumps2MergeCol, &tmpNumRows);
            std::cout << "Read input column\n";
            
            double *numPxls = new double[numRows];
            
            double **meanVals = new double*[numSpecBands];
            double **sumVals = new double*[numSpecBands];
            
            for(int i = 0; i < numSpecBands; ++i)
            {
                tmpNumRows = 0;
                meanVals[i] = attUtils.readDoubleColumn(rat, "Mean"+colNames.at(i), &tmpNumRows);
                if(tmpNumRows != numRows)
                {
                    throw rsgis::img::RSGISImageCalcException("Number of rows was incorrect. (Mean)");
                }
                tmpNumRows = 0;
                sumVals[i] = attUtils.readDoubleColumn(rat, "Sum"+colNames.at(i), &tmpNumRows);
                if(tmpNumRows != numRows)
                {
                    throw rsgis::img::RSGISImageCalcException("Number of rows was incorrect. (Sum)");
                }
            }
            
            int *clumpID = new int[numRows];
            int *clumpIDUp = new int[numRows];
            bool *updated = new bool[numRows];
            std::vector<size_t> tmpNeigh;
            for(size_t i = 0; i < numRows; ++i)
            {
                clumpID[i] = i;
                clumpIDUp[i] = i;
                updated[i] = false;
                numPxls[i] = sumVals[0][i] / meanVals[0][i];
            }
            
            double *valsRef = new double[numSpecBands];
            double *valsTest = new double[numSpecBands];
            double val = 0.0;
            bool process = true;
            bool first = true;
            size_t minIdx = 0;
            double minVal = 0.0;
            bool found = false;
            unsigned int processIter = 1;
            while(process)
            {
                std::cout << "Processing Iteration " << processIter++ << std::endl;
                process = false;
                for(size_t i = 0; i < numRows; ++i)
                {
                    updated[i] = false;
                }
                for(size_t i = 0; i < numRows; ++i)
                {
                    if(selectCol[i] == 1)
                    {
                        //std::cout << "\t Process clump " << i;
                        first = true;
                        for(int n = 0; n < numSpecBands; ++n)
                        {
                            valsRef[n] = meanVals[n][i];
                        }
                        for(std::vector<size_t>::iterator iterNeigh = neighbours->at(i)->begin(); iterNeigh != neighbours->at(i)->end(); ++iterNeigh)
                        {
                            if(selectCol[(*iterNeigh)] != 1)
                            {
                                for(int n = 0; n < numSpecBands; ++n)
                                {
                                    valsTest[n] = meanVals[n][(*iterNeigh)];
                                }
                                
                                val = this->calcDist(valsRef, valsTest, numSpecBands);
                                if(first)
                                {
                                    minIdx = (*iterNeigh);
                                    minVal = val;
                                    first = false;
                                }
                                else if(val < minVal)
                                {
                                    minIdx = (*iterNeigh);
                                    minVal = val;
                                }
                            }
                        }
                        
                        if(!first)
                        {
                            updated[i] = true;
                            clumpIDUp[i] = minIdx;
                            //std::cout << " is merged with " << minIdx;
                            process = true;
                        }
                        //std::cout << std::endl;
                    }
                }
                
                //std::cout << "Found What to Merge to...\n";
                
                for(size_t i = 0; i < numRows; ++i)
                {
                    if(updated[i])
                    {
                        //std::cout << "Updating " << i << " to " << clumpIDUp[i] << std::endl;
                        numPxls[clumpIDUp[i]] += numPxls[i];
                        for(int n = 0; n < numSpecBands; ++n)
                        {
                            sumVals[n][clumpIDUp[i]] += sumVals[n][i];
                            meanVals[n][clumpIDUp[i]] = sumVals[n][clumpIDUp[i]]/((double)numPxls[clumpIDUp[i]]);
                        }
                        
                        tmpNeigh.clear();
                        for(std::vector<size_t>::iterator iterNeighRef = neighbours->at(i)->begin(); iterNeighRef != neighbours->at(i)->end(); ++iterNeighRef)
                        {
                            found = false;
                            for(std::vector<size_t>::iterator iterNeighObj = neighbours->at(clumpIDUp[i])->begin(); iterNeighObj != neighbours->at(clumpIDUp[i])->end(); ++iterNeighObj)
                            {
                                if( (*iterNeighRef) == (*iterNeighObj) )
                                {
                                    found = true;
                                    break;
                                }
                            }
                            if(!found)
                            {
                                tmpNeigh.push_back(*iterNeighRef);
                            }
                        }
                        
                        for(std::vector<size_t>::iterator iterNeigh = tmpNeigh.begin(); iterNeigh != tmpNeigh.end(); ++iterNeigh)
                        {
                            neighbours->at(clumpIDUp[i])->push_back(*iterNeigh);
                        }
                        neighbours->at(i)->clear();
                        selectCol[i] = 0;
                        updated[i] = false;
                    }
                }
                //std::cout << "Iteration Complete\n";
            }
            std::cout << "Completed Iterations\n";
            attUtils.writeIntColumn(rat, "OutClumpIDs", clumpIDUp, numRows);
            
            delete[] valsRef;
            delete[] valsTest;
            
            
            for(int i = 0; i < numSpecBands; ++i)
            {
                delete[] meanVals[i];
                delete[] sumVals[i];
            }
            delete[] meanVals;
            delete[] sumVals;
            delete[] numPxls;
            delete[] selectCol;
            delete[] updated;
            delete[] clumpIDUp;
        }
        catch(rsgis::img::RSGISImageCalcException &e)
        {
            throw e;
        }
        catch (rsgis::RSGISException &e)
        {
            throw rsgis::img::RSGISImageCalcException(e.what());
        }
        catch (std::exception &e)
        {
            throw rsgis::img::RSGISImageCalcException(e.what());
        }
    }
    */

    RSGISMergeSegments::~RSGISMergeSegments()
    {
        
    }
    
}}

