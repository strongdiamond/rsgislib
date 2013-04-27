/*
 *  RSGISSampleImage.h
 *  RSGIS_LIB
 *
 *  Created by Pete Bunting on 04/04/2013.
 *  Copyright 2013 RSGISLib.
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

#ifndef RSGISSampleImage_H
#define RSGISSampleImage_H

#include <iostream>
#include <fstream>

#include "img/RSGISImageCalcException.h"
#include "img/RSGISCalcImage.h"
#include "img/RSGISCalcImageValue.h"
#include "img/RSGISImageBandException.h"
#include "img/RSGISImageCalcException.h"

#include "utils/RSGISExportData2HDF.h"
#include "math/RSGISMathsUtils.h"

#include "gdal_priv.h"

namespace rsgis{namespace img{

    class RSGISSampleImage
    {
    public:
        RSGISSampleImage();
        void subSampleImage(GDALDataset *inputImage, std::string outputFile, unsigned int sample, float noData, bool useNoData)throw(RSGISImageException);
        ~RSGISSampleImage();
    };
    
    class RSGISSampleCalcImage : public RSGISCalcImageValue
    {
    public:
        RSGISSampleCalcImage(unsigned int sample, float noData, bool useNoData, rsgis::utils::RSGISExportColumnData2HDF *dataExport, float *dataRow);
        void calcImageValue(float *bandValues, int numBands, float *output) throw(RSGISImageCalcException){throw RSGISImageCalcException("No implemented");};
        void calcImageValue(float *bandValues, int numBands) throw(RSGISImageCalcException);
        void calcImageValue(float *bandValues, int numBands, geos::geom::Envelope extent) throw(RSGISImageCalcException){throw RSGISImageCalcException("No implemented");};
        void calcImageValue(float *bandValues, int numBands, float *output, geos::geom::Envelope extent) throw(RSGISImageCalcException){throw RSGISImageCalcException("No implemented");};
        void calcImageValue(float ***dataBlock, int numBands, int winSize, float *output) throw(RSGISImageCalcException){throw RSGISImageCalcException("No implemented");};
        void calcImageValue(float ***dataBlock, int numBands, int winSize, float *output, geos::geom::Envelope extent) throw(RSGISImageCalcException){throw RSGISImageCalcException("No implemented");};
        bool calcImageValueCondition(float ***dataBlock, int numBands, int winSize, float *output) throw(RSGISImageCalcException){throw RSGISImageCalcException("No implemented");};
        void reset(){this->currentPxl = 1;};
        ~RSGISSampleCalcImage();
    private:
        rsgis::utils::RSGISExportColumnData2HDF *dataExport;
        unsigned int sample;
        float noData;
        bool useNoData;
        unsigned int currentPxl;
        float *dataRow;
    };

}}

#endif

