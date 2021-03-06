/************************************************************************/
/*                                                                      */
/*               Copyright 2008-2017 by Benjamin Seppke                 */
/*       Cognitive Systems Group, University of Hamburg, Germany        */
/*                                                                      */
/*    This file is part of the GrAphical Image Processing Enviroment.   */
/*    The GRAIPE Website may be found at:                               */
/*        https://github.com/bseppke/graipe                             */
/*    Please direct questions, bug reports, and contributions to        */
/*    the GitHub page and use the methods provided there.               */
/*                                                                      */
/*    Permission is hereby granted, free of charge, to any person       */
/*    obtaining a copy of this software and associated documentation    */
/*    files (the "Software"), to deal in the Software without           */
/*    restriction, including without limitation the rights to use,      */
/*    copy, modify, merge, publish, distribute, sublicense, and/or      */
/*    sell copies of the Software, and to permit persons to whom the    */
/*    Software is furnished to do so, subject to the following          */
/*    conditions:                                                       */
/*                                                                      */
/*    The above copyright notice and this permission notice shall be    */
/*    included in all copies or substantial portions of the             */
/*    Software.                                                         */
/*                                                                      */
/*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND    */
/*    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES   */
/*    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND          */
/*    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT       */
/*    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,      */
/*    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR     */
/*    OTHER DEALINGS IN THE SOFTWARE.                                   */
/*                                                                      */
/************************************************************************/

#include "images/image.hxx"
#include "core/core.h"

#include <vigra/specklefilters.hxx>
#include <vigra/shockfilter.hxx>
#include <vigra/medianfilter.hxx>

namespace graipe {

/**
 * @addtogroup graipe_imagefilters
 * @{
 *
 * @file
 * @brief Implementation file for the image filters' module
 */
 
/**
 * This class is the base class for all (window/convolution-based)
 * Image filters. It provides the selection of a border mode, but does
 * not implement any specific running phase.
 */
class ImageFilter
:   public Algorithm
{
    public:
        /**
         * Default_constructor. Introduces the list of vigra's 
         * BorderTreatmentModes.
         */
        ImageFilter(Workspace * wsp)
        : Algorithm(wsp)
        {
            //According to VIGRA's <vigra/bordertreatment.hxx>:
            /*
            enum BorderTreatmentMode
            {
                  // do not operate on a pixel where the kernel does 
                  // not fit in the image
               BORDER_TREATMENT_AVOID, 

                  // clip kernel at image border (this is only useful if the
                  //  kernel is >= 0 everywhere)
               BORDER_TREATMENT_CLIP, 

                  // repeat the nearest valid pixel
               BORDER_TREATMENT_REPEAT,

                  // reflect image at last row/column 
               BORDER_TREATMENT_REFLECT, 

                  // wrap image around (periodic boundary conditions)
               BORDER_TREATMENT_WRAP

                  // assume that all outside points have value zero
               BORDER_TREATMENT_ZEROPAD
            };
            */
            
            m_border_treatment_modes.append("AVOID \tIgnore the borders");
            m_border_treatment_modes.append("");
            m_border_treatment_modes.append("REPEAT \tlast rows and columns at border");
            m_border_treatment_modes.append("REFLECT \timage intensities at border");
            m_border_treatment_modes.append("WRAP \timage intensities around border");
            m_border_treatment_modes.append("ZEROPAD \t filling of borders");
        }
    
    protected:
       /** Class member for the reatment modes **/
       QStringList m_border_treatment_modes;
};




/**
 * This class is an implementation of the Frost Filter for anisotropic filtering.
 * The filter is typically applied on images, which suffer from speckle noise.
 */
class FrostFilter
:   public ImageFilter
{
    public:
        /**
         * Default constructor. Adds all neccessary parameters for this algorithm to run.
         */
        FrostFilter(Workspace * wsp)
        : ImageFilter(wsp)
        {
            m_parameters->addParameter("image", new ModelParameter("Image",	"Image", NULL, false, wsp));
            m_parameters->addParameter("size", new IntParameter("Filter window size", 1, 9999, 11));
            m_parameters->addParameter("k", new FloatParameter("Damping factor k", 0, 1, 1));
            m_parameters->addParameter("bt", new EnumParameter("Border treatment", m_border_treatment_modes, 2));
            m_results.push_back(new Image<float>(wsp));
        }
        
        /**
         * Returns the name of this algorithm.
         * 
         * \return Always: "FrostFilter"
         */
        QString typeName() const
        {
            return "FrostFilter";
        }
        
        /**
         * Specialization of the running phase of this algorithm.
         */
        void run()
        {
            if(!parametersValid())
            {
                //Parameters set incorrectly
                emit errorMessage(QString("Some parameters are not available"));
            }
            else
            {
                lockModels();
                try
                {
                    emit statusMessage(0.0, QString("started"));
                    
                    ModelParameter	* param_image      = static_cast<ModelParameter*> ((*m_parameters)["image"]);
                    IntParameter	* param_windowSize = static_cast<IntParameter*>((*m_parameters)["size"]);
                    FloatParameter	* param_damping_k  = static_cast<FloatParameter*>((*m_parameters)["k"]);
                    EnumParameter	* param_btmode     = static_cast<EnumParameter*> ((*m_parameters)["bt"]);
                    
                    Image<float>* current_image = static_cast<Image<float>*>(param_image->value());
                    
                    emit statusMessage(1.0, QString("starting computation"));
                    
                    //create new image and do the transform
                    Image<float>* new_image = new Image<float>(current_image->size(), current_image->numBands(), m_workspace);
                    
                    //copy metadata from current image (will be overwritten later)
                    current_image->copyMetadata(*new_image);
                    
                    new_image->setName(QString("Frost Filtered ") + current_image->name());
                    
                    m_phase_count = current_image->numBands();
                    
                    for( m_phase=0; m_phase < m_phase_count; m_phase++)
                    {	
                        frostFilter(current_image->band(m_phase),
                                    new_image->band(m_phase),
                                    vigra::Diff2D(param_windowSize->value(),param_windowSize->value()),
                                    param_damping_k->value(),
                                    vigra::BorderTreatmentMode(param_btmode->value()));
                                    
                        emit statusMessage(m_phase*99.0/m_phase_count, QString("filtering"));
                    }
                    
                    QString descr("The following parameters were used for filtering:\n");
                    descr += m_parameters->valueText("ModelParameter");
                    new_image->setDescription(descr);
                                
                    delete m_results[0];
                    m_results[0] = new_image;
                    
                    emit statusMessage(100.0, QString("finished computation"));
                    emit finished();
                }
                catch(std::exception& e)
                {
                    emit errorMessage(QString("Explainable error occured: ") + QString::fromStdString(e.what()));
                }
                catch(...)
                {
                    emit errorMessage(QString("Non-explainable error occured"));		
                }
                unlockModels();
            }
        }
};

/** 
 * Creates one instance of the FrostFilter
 * algorithm defined above.
 *
 * \return A new instance of the FrostFilter.
 */
Algorithm* createFrostFilter(Workspace * wsp)
{
	return new FrostFilter(wsp);
}




/**
 * This class is an implementation of the Enhanced Frost Filter for anisotropic filtering.
 * The filter is typically applied on images, which suffer from speckle noise.
 */
class EnhancedFrostFilter
:   public ImageFilter
{
    public:
        /**
         * Default constructor. Adds all neccessary parameters for this algorithm to run.
         */
        EnhancedFrostFilter(Workspace * wsp)
        : ImageFilter(wsp)
        {
            m_parameters->addParameter("image", new ModelParameter("Image",	"Image", NULL, false, wsp));
            m_parameters->addParameter("size", new IntParameter("Filter window size", 1, 9999, 11));
            m_parameters->addParameter("k", new FloatParameter("Damping factor k", 0, 1, 1));
            m_parameters->addParameter("ENL", new IntParameter("Equivalent Number of looks (ENL)", 1, 100, 4));
            m_parameters->addParameter("bt", new EnumParameter("Border treatment", m_border_treatment_modes, 2));
            m_results.push_back(new Image<float>(wsp));
        }
        
        /**
         * Returns the name of this algorithm.
         * 
         * \return Always: "EnhancedFrostFilter"
         */
        QString typeName() const
        {
            return "EnhancedFrostFilter";
        }
    
        /**
         * Specialization of the running phase of this algorithm.
         */
        void run()
        {
            if(!parametersValid())
            {
                //Parameters set incorrectly
                emit errorMessage(QString("Some parameters are not available"));
            }
            else
            {
                lockModels();
                try
                {
                    emit statusMessage(0.0, QString("started"));
                    
                    ModelParameter	* param_image      = static_cast<ModelParameter*> ((*m_parameters)["image"]);
                    IntParameter	* param_windowSize = static_cast<IntParameter*>((*m_parameters)["size"]);
                    FloatParameter	* param_damping_k  = static_cast<FloatParameter*>((*m_parameters)["k"]);
                    IntParameter	* param_enl		   = static_cast<IntParameter*>((*m_parameters)["ENL"]);
                    EnumParameter	* param_btmode     = static_cast<EnumParameter*> ((*m_parameters)["bt"]);
                    
                    Image<float>* current_image = static_cast<Image<float>*>(param_image->value());
                    
                    emit statusMessage(1.0, QString("starting computation"));
                    
                    //create new image and do the transform
                    Image<float>* new_image = new Image<float>(current_image->size(), current_image->numBands(), m_workspace);
                    
                    //copy metadata from current image (will be overwritten later)
                    current_image->copyMetadata(*new_image);
                                
                    new_image->setName(QString("Enh. Frost Filtered ") + current_image->name());
                    
                    m_phase_count = current_image->numBands();
                    
                    for( m_phase=0; m_phase < m_phase_count; m_phase++)
                    {	
                        enhancedFrostFilter(current_image->band(m_phase),
                                            new_image->band(m_phase),
                                            vigra::Diff2D(param_windowSize->value(), param_windowSize->value()),
                                            param_damping_k->value(), param_enl->value(),
                                            vigra::BorderTreatmentMode(param_btmode->value()));
                        
                        emit statusMessage(m_phase*99.0/m_phase_count, QString("filtering"));
                    }
                    
                    QString descr("The following parameters were used for filtering:\n");
                    descr += m_parameters->valueText("ModelParameter");
                    new_image->setDescription(descr);
                                
                    delete m_results[0];
                    m_results[0] = new_image;
                    
                    emit statusMessage(100.0, QString("finished computation"));
                    emit finished();
                }
                catch(std::exception& e)
                {
                    emit errorMessage(QString("Explainable error occured: ") + QString::fromStdString(e.what()));
                }
                catch(...)
                {
                    emit errorMessage(QString("Non-explainable error occured"));		
                }
                unlockModels();
            }
        }
};

/** 
 * Creates one instance of the EnhancedFrostFilter
 * algorithm defined above.
 *
 * \return A new instance of the EnhancedFrostFilter.
 */
Algorithm* createEnhancedFrostFilter(Workspace * wsp)
{
	return new EnhancedFrostFilter(wsp);
}





/**
 * This class is an implementation of the Gamma Maximum A Posterior Filter
 * for anisotropic filtering of images.
 * The filter is typically applied on images, which suffer from speckle noise.
 */
class GammaMAPFilter
:   public ImageFilter
{
    public:
        /**
         * Default constructor. Adds all neccessary parameters for this algorithm to run.
         */
        GammaMAPFilter(Workspace * wsp)
        : ImageFilter(wsp)
        {
            m_parameters->addParameter("image", new ModelParameter("Image",	"Image", NULL, false, wsp));
            m_parameters->addParameter("size", new IntParameter("Filter window size", 1, 9999, 11));
            m_parameters->addParameter("ENL", new IntParameter("Equivalent Number of looks (ENL)", 1, 100, 4));
            m_parameters->addParameter("bt", new EnumParameter("Border treatment", m_border_treatment_modes, 2));
            m_results.push_back(new Image<float>(wsp));
        }
    
        /**
         * Returns the name of this algorithm.
         * 
         * \return Always: "GammaMAPFilter"
         */
        QString typeName() const
        {
            return "GammaMAPFilter";
        }
    
        /**
         * Specialization of the running phase of this algorithm.
         */
        void run()
        {
            if(!parametersValid())
            {
                //Parameters set incorrectly
                emit errorMessage(QString("Some parameters are not available"));
            }
            else
            {
                lockModels();
                try
                {
                    emit statusMessage(0.0, QString("started"));
                    
                    ModelParameter	* param_image      = static_cast<ModelParameter*> ((*m_parameters)["image"]);
                    IntParameter	* param_windowSize = static_cast<IntParameter*>((*m_parameters)["size"]);
                    FloatParameter	* param_enl        = static_cast<FloatParameter*>((*m_parameters)["ENL"]);
                    EnumParameter	* param_btmode     = static_cast<EnumParameter*> ((*m_parameters)["bt"]);
                    
                    Image<float>* current_image = static_cast<Image<float>*>(param_image->value());
                    
                    emit statusMessage(1.0, QString("starting computation"));
                    
                    //create new image and do the transform
                    Image<float>* new_image = new Image<float>(current_image->size(), current_image->numBands(), m_workspace);
                    
                    //copy metadata from current image (will be overwritten later)
                    current_image->copyMetadata(*new_image);
                    
                    new_image->setName(QString("Gamma Filtered ") + current_image->name());
                    
                    m_phase_count = current_image->numBands();
                    
                    for( m_phase=0; m_phase < m_phase_count; m_phase++)
                    {	
                        gammaMAPFilter(current_image->band(m_phase),
                                       new_image->band(m_phase),
                                       vigra::Diff2D(param_windowSize->value(), param_windowSize->value()),
                                       param_enl->value(),
                                       vigra::BorderTreatmentMode(param_btmode->value()));
                                       
                        emit statusMessage(m_phase*99.0/m_phase_count, QString("filtering"));
                    }
                    
                    QString descr("The following parameters were used for filtering:\n");
                    descr += m_parameters->valueText("ModelParameter");
                    new_image->setDescription(descr);
                                
                    delete m_results[0];
                    m_results[0] = new_image;
                    
                    emit statusMessage(100.0, QString("finished computation"));
                    emit finished();
                }
                catch(std::exception& e)
                {
                    emit errorMessage(QString("Explainable error occured: ") + QString::fromStdString(e.what()));
                }
                catch(...)
                {
                    emit errorMessage(QString("Non-explainable error occured"));		
                }
                unlockModels();
            }
        }
};

/** 
 * Creates one instance of the GammaMAPFilter
 * algorithm defined above.
 *
 * \return A new instance of the GammaMAPFilter.
 */
Algorithm* createGammaMAPFilter(Workspace * wsp)
{
	return new GammaMAPFilter(wsp);
}




/**
 * This class is an implementation of the Kuan Filter for anisotropic filtering of images.
 * The filter is typically applied on images, which suffer from speckle noise.
 */
class KuanFilter
:   public ImageFilter
{
    public:
        /**
         * Default constructor. Adds all neccessary parameters for this algorithm to run.
         */
        KuanFilter(Workspace * wsp)
        : ImageFilter(wsp)
        {
            m_parameters->addParameter("image", new ModelParameter("Image",	"Image", NULL, false, wsp));
            m_parameters->addParameter("size", new IntParameter("Filter window size", 1, 9999, 11));
            m_parameters->addParameter("ENL", new IntParameter("Equivalent Number of looks (ENL)", 1, 100, 4));
            m_parameters->addParameter("bt", new EnumParameter("Border treatment", m_border_treatment_modes, 2));
            m_results.push_back(new Image<float>(wsp));
        }
    
        /**
         * Returns the name of this algorithm.
         * 
         * \return Always: "KuanFilter"
         */
        QString typeName() const
        {
            return "KuanFilter";
        }
        
        /**
         * Specialization of the running phase of this algorithm.
         */
        void run()
        {
            if(!parametersValid())
            {
                //Parameters set incorrectly
                emit errorMessage(QString("Some parameters are not available"));
            }
            else
            {
                lockModels();
                try
                {
                    emit statusMessage(0.0, QString("started"));
                    
                    ModelParameter	* param_image      = static_cast<ModelParameter*> ((*m_parameters)["image"]);
                    IntParameter	* param_windowSize = static_cast<IntParameter*>((*m_parameters)["size"]);
                    IntParameter	* param_enl		   = static_cast<IntParameter*>((*m_parameters)["ENL"]);
                    EnumParameter	* param_btmode     = static_cast<EnumParameter*> ((*m_parameters)["bt"]);
                    
                    Image<float>* current_image = static_cast<Image<float>*>(param_image->value());
                    
                    emit statusMessage(1.0, QString("starting computation"));
                    
                    //create new image and do the transform
                    Image<float>* new_image = new Image<float>(current_image->size(), current_image->numBands(), m_workspace);
                    
                    //copy metadata from current image (will be overwritten later)
                    current_image->copyMetadata(*new_image);
                    
                    new_image->setName(QString("Kuan Filtered ") + current_image->name());
                    
                    m_phase_count = current_image->numBands();
                    
                    for( m_phase=0; m_phase < m_phase_count; m_phase++)
                    {	
                        kuanFilter(current_image->band(m_phase),
                                   new_image->band(m_phase),
                                   vigra::Diff2D(param_windowSize->value(), param_windowSize->value()),
                                   param_enl->value(),
                                   vigra::BorderTreatmentMode(param_btmode->value()));
                        
                        emit statusMessage(m_phase*99.0/m_phase_count, QString("filtering"));
                    }
                    
                    QString descr("The following parameters were used for filtering:\n");
                    descr += m_parameters->valueText("ModelParameter");
                    new_image->setDescription(descr);
                                
                    delete m_results[0];
                    m_results[0] = new_image;
                    
                    emit statusMessage(100.0, QString("finished computation"));
                    emit finished();
                }
                catch(std::exception& e)
                {
                    emit errorMessage(QString("Explainable error occured: ") + QString::fromStdString(e.what()));
                }
                catch(...)
                {
                    emit errorMessage(QString("Non-explainable error occured"));		
                }
                unlockModels();
            }
        }
};

/** 
 * Creates one instance of the KuanFilter
 * algorithm defined above.
 *
 * \return A new instance of the KuanFilter.
 */
Algorithm* createKuanFilter(Workspace * wsp)
{
	return new KuanFilter(wsp);
}




/**
 * This class is an implementation of the Lee Filter for anisotropic filtering of images.
 * The filter is typically applied on images, which suffer from speckle noise.
 */
class LeeFilter
:   public ImageFilter
{
    public:
        /**
         * Default constructor. Adds all neccessary parameters for this algorithm to run.
         */
        LeeFilter(Workspace * wsp)
        : ImageFilter(wsp)
        {
            m_parameters->addParameter("image", new ModelParameter("Image",	"Image", NULL, false, wsp));
            m_parameters->addParameter("size", new IntParameter("Filter window size", 1, 9999, 11));
            m_parameters->addParameter("ENL", new IntParameter("Equivalent Number of looks (ENL)", 1, 100, 4));
            m_parameters->addParameter("bt", new EnumParameter("Border treatment", m_border_treatment_modes, 2));
            m_results.push_back(new Image<float>(wsp));
        }
        
        /**
         * Returns the name of this algorithm.
         * 
         * \return Always: "LeeFilter"
         */
        QString typeName() const
        {
            return "LeeFilter";
        }
    
        /**
         * Specialization of the running phase of this algorithm.
         */
        void run()
        {
            if(!parametersValid())
            {
                //Parameters set incorrectly
                emit errorMessage(QString("Some parameters are not available"));
            }
            else
            {
                lockModels();
                try
                {
                    emit statusMessage(0.0, QString("started"));
                    
                    ModelParameter	* param_image      = static_cast<ModelParameter*> ((*m_parameters)["image"]);
                    IntParameter	* param_windowSize = static_cast<IntParameter*>((*m_parameters)["size"]);
                    IntParameter	* param_enl		   = static_cast<IntParameter*>((*m_parameters)["ENL"]);
                    EnumParameter	* param_btmode     = static_cast<EnumParameter*> ((*m_parameters)["bt"]);
                    
                    Image<float>* current_image = static_cast<Image<float>*>(param_image->value());
                    
                    emit statusMessage(1.0, QString("starting computation"));
                    
                    //create new image and do the transform
                    Image<float>* new_image = new Image<float>(current_image->size(), current_image->numBands(), m_workspace);
                    
                    //copy metadata from current image (will be overwritten later)
                    current_image->copyMetadata(*new_image);
                    
                    new_image->setName(QString("Lee Filtered ") + current_image->name());
                    
                    m_phase_count = current_image->numBands();
                    
                    for( m_phase=0; m_phase < m_phase_count; m_phase++)
                    {	
                        leeFilter(current_image->band(m_phase),
                                  new_image->band(m_phase),
                                  vigra::Diff2D(param_windowSize->value(), param_windowSize->value()),
                                  param_enl->value(),
                                  vigra::BorderTreatmentMode(param_btmode->value()));
                        
                        emit statusMessage(m_phase*99.0/m_phase_count, QString("filtering"));
                    }
                    
                    QString descr("The following parameters were used for filtering:\n");
                    descr += m_parameters->valueText("ModelParameter");
                    new_image->setDescription(descr);
                                
                    delete m_results[0];
                    m_results[0] = new_image;
                    
                    emit statusMessage(100.0, QString("finished computation"));
                    emit finished();
                }
                catch(std::exception& e)
                {
                    emit errorMessage(QString("Explainable error occured: ") + QString::fromStdString(e.what()));
                }
                catch(...)
                {
                    emit errorMessage(QString("Non-explainable error occured"));		
                }
                unlockModels();
            }
        }
};

/** 
 * Creates one instance of the LeeFilter
 * algorithm defined above.
 *
 * \return A new instance of the LeeFilter.
 */
Algorithm* createLeeFilter(Workspace * wsp)
{
	return new LeeFilter(wsp);
}







/**
 * This class is an implementation of the Enhanced Lee Filter for anisotropic filtering of images.
 * The filter is typically applied on images, which suffer from speckle noise.
 */
class EnhancedLeeFilter
:   public ImageFilter
{
    public:
        /**
         * Default constructor. Adds all neccessary parameters for this algorithm to run.
         */
        EnhancedLeeFilter(Workspace * wsp)
        : ImageFilter(wsp)
        {
            m_parameters->addParameter("image", new ModelParameter("Image",	"Image", NULL, false, wsp));
            m_parameters->addParameter("size", new IntParameter("Filter window size", 1, 9999, 11));
            m_parameters->addParameter("k", new FloatParameter("Damping factor k", 0, 1, 1));
            m_parameters->addParameter("ENL", new IntParameter("Equivalent Number of looks (ENL)", 1, 100, 4));
            m_parameters->addParameter("bt", new EnumParameter("Border treatment", m_border_treatment_modes, 2));
            m_results.push_back(new Image<float>(wsp));
        }
    
        /**
         * Returns the name of this algorithm.
         * 
         * \return Always: "EnhancedLeeFilter"
         */
        QString typeName() const
        {
            return "EnhancedLeeFilter";
        }
    
        /**
         * Specialization of the running phase of this algorithm.
         */
        void run()
        {
            if(!parametersValid())
            {
                //Parameters set incorrectly
                emit errorMessage(QString("Some parameters are not available"));
            }
            else
            {
                lockModels();
                try
                {
                    emit statusMessage(0.0, QString("started"));
                    
                    ModelParameter	* param_image      = static_cast<ModelParameter*> ((*m_parameters)["image"]);
                    IntParameter	* param_windowSize = static_cast<IntParameter*>((*m_parameters)["size"]);
                    FloatParameter	* param_damping_k  = static_cast<FloatParameter*>((*m_parameters)["k"]);
                    IntParameter	* param_enl		   = static_cast<IntParameter*>((*m_parameters)["ENL"]);
                    EnumParameter	* param_btmode     = static_cast<EnumParameter*> ((*m_parameters)["bt"]);
                    
                    Image<float>* current_image = static_cast<Image<float>*>(  param_image->value() );	
                    
                    emit statusMessage(1.0, QString("starting computation"));
                    
                    //create new image and do the transform
                    Image<float>* new_image = new Image<float>(current_image->size(), current_image->numBands(), m_workspace);
                    
                    //copy metadata from current image (will be overwritten later)
                    current_image->copyMetadata(*new_image);
                    
                    new_image->setName(QString("Enh. Lee Filtered ") + current_image->name());
                    
                    m_phase_count = current_image->numBands();
                    
                    for( m_phase=0; m_phase < m_phase_count; m_phase++)
                    {	
                        enhancedLeeFilter(current_image->band(m_phase),
                                          new_image->band(m_phase),
                                          vigra::Diff2D(param_windowSize->value(), param_windowSize->value()),
                                          param_damping_k->value(), param_enl->value(),
                                          vigra::BorderTreatmentMode(param_btmode->value()));
                        
                        emit statusMessage(m_phase*99.0/m_phase_count, QString("filtering"));
                    }
                    
                    QString descr("The following parameters were used for filtering:\n");
                    descr += m_parameters->valueText("ModelParameter");
                    new_image->setDescription(descr);
                                
                    delete m_results[0];
                    m_results[0] = new_image;
                    
                    emit statusMessage(100.0, QString("finished computation"));
                    emit finished();
                }
                catch(std::exception& e)
                {
                    emit errorMessage(QString("Explainable error occured: ") + QString::fromStdString(e.what()));
                }
                catch(...)
                {
                    emit errorMessage(QString("Non-explainable error occured"));		
                }
                unlockModels();
            }
        }
};

/** 
 * Creates one instance of the EnhancedLeeFilter
 * algorithm defined above.
 *
 * \return A new instance of the EnhancedLeeFilter.
 */
Algorithm* createEnhancedLeeFilter(Workspace * wsp)
{
	return new EnhancedLeeFilter(wsp);
}




/**
 * This class is an implementation of the classical anisotropic median filter.
 * The filter is typically applied on images, which suffer from non-linear noise.
 */
class MedianFilter
:   public ImageFilter
{
    public:
        /**
         * Default constructor. Adds all neccessary parameters for this algorithm to run.
         */
        MedianFilter(Workspace * wsp)
        : ImageFilter(wsp)
        {
            m_parameters->addParameter("image", new ModelParameter("Image",	"Image", NULL, false, wsp));
            m_parameters->addParameter("size", new IntParameter("Filter window size", 1, 9999, 11));
            m_parameters->addParameter("bt", new EnumParameter("Border treatment", m_border_treatment_modes, 2));
            m_results.push_back(new Image<float>(wsp));
        }
    
        /**
         * Returns the name of this algorithm.
         * 
         * \return Always: "MedianFilter"
         */
        QString typeName() const
        {
            return "MedianFilter";
        }
        
        /**
         * Specialization of the running phase of this algorithm.
         */
        void run()
        {
            if(!parametersValid())
            {
                //Parameters set incorrectly
                emit errorMessage(QString("Some parameters are not available"));
            }
            else
            {
                lockModels();
                try
                {
                    emit statusMessage(0.0, QString("started"));
                    
                    ModelParameter	* param_image      = static_cast<ModelParameter*> ((*m_parameters)["image"]);
                    IntParameter	* param_windowSize = static_cast<IntParameter*>((*m_parameters)["size"]);
                    EnumParameter	* param_btmode     = static_cast<EnumParameter*> ((*m_parameters)["bt"]);
                    
                    Image<float>* current_image = static_cast<Image<float>*>(  param_image->value() );	
                    
                    emit statusMessage(1.0, QString("starting computation"));
                    
                    //create new image and do the transform
                    Image<float>* new_image = new Image<float>(current_image->size(), current_image->numBands(), m_workspace);
                    
                    //copy metadata from current image (will be overwritten later)
                    current_image->copyMetadata(*new_image);
                    
                    new_image->setName(QString("Median Filtered ") + current_image->name());
                    
                    m_phase_count = current_image->numBands();
                    
                    for( m_phase=0; m_phase < m_phase_count; m_phase++)
                    {	
                        medianFilter(current_image->band(m_phase),
                                     new_image->band(m_phase),
                                     vigra::Diff2D(param_windowSize->value(), param_windowSize->value()),
                                     vigra::BorderTreatmentMode(param_btmode->value()));
                                             
                        emit statusMessage(m_phase*99.0/m_phase_count, QString("filtering"));
                    }
                    
                    QString descr("The following parameters were used for filtering:\n");
                    descr += m_parameters->valueText("ModelParameter");
                    new_image->setDescription(descr);
                                
                    delete m_results[0];
                    m_results[0] = new_image;
                    
                    emit statusMessage(100.0, QString("finished computation"));
                    emit finished();
                }
                catch(std::exception& e)
                {
                    emit errorMessage(QString("Explainable error occured: ") + QString::fromStdString(e.what()));
                }
                catch(...)
                {
                    emit errorMessage(QString("Non-explainable error occured"));		
                }
                unlockModels();
            }
        }
};

/** 
 * Creates one instance of the MedianFilter
 * algorithm defined above.
 *
 * \return A new instance of the MedianFilter.
 */
Algorithm* createMedianFilter(Workspace * wsp)
{
	return new MedianFilter(wsp);
}




/**
 * This class is an implementation of the Coherence Enhancing Shock Filter for anisotropic filtering of images.
 * The filter is more like an artistic filter, since it enhances contour following in the images.
 */
class ShockFilter
:   public Algorithm
{
    public:
        /**
         * Default constructor. Adds all neccessary parameters for this algorithm to run.
         */    
        ShockFilter(Workspace * wsp)
        : Algorithm(wsp)
        {
            m_parameters->addParameter("image", new ModelParameter("Image",	"Image", NULL, false, wsp));
            m_parameters->addParameter("sigma1", new FloatParameter("inner Sigma", 0.0, 100, 0.7f));
            m_parameters->addParameter("sigma2", new FloatParameter("outer Sigma", 0.0, 100, 3));
            m_parameters->addParameter("upwind", new FloatParameter("upwinding factor", 0.0, 10.0, 0.3f));
            m_parameters->addParameter("iterations", new IntParameter("Iterations", 1, 9999, 10));
            m_results.push_back(new Image<float>(wsp));
        }
    
        /**
         * Returns the name of this algorithm.
         * 
         * \return Always: "ShockFilter"
         */
        QString typeName() const
        {
            return "ShockFilter";
        }
        
        /**
         * Specialization of the running phase of this algorithm.
         */
        void run()
        {
            if(!parametersValid())
            {
                //Parameters set incorrectly
                emit errorMessage(QString("Some parameters are not available"));
            }
            else
            {
                lockModels();
                try
                {
                    emit statusMessage(0.0, QString("started"));
                    
                    ModelParameter	* param_image      = static_cast<ModelParameter*> ((*m_parameters)["image"]);
                    FloatParameter	* param_iSigma     = static_cast<FloatParameter*>((*m_parameters)["sigma1"]);
                    FloatParameter	* param_oSigma     = static_cast<FloatParameter*>((*m_parameters)["sigma2"]);
                    FloatParameter	* param_upwind     = static_cast<FloatParameter*>((*m_parameters)["upwind"]);
                    IntParameter	* param_iterations = static_cast<IntParameter*>((*m_parameters)["iterations"]);
                    
                    Image<float>* current_image = static_cast<Image<float>*>(  param_image->value() );	
                    
                    emit statusMessage(1.0, QString("starting computation"));
                    
                    //create new image and do the transform
                    Image<float>* new_image = new Image<float>(current_image->size(), current_image->numBands(), m_workspace);
                    
                    //copy metadata from current image (will be overwritten later)
                    current_image->copyMetadata(*new_image);
                    
                    new_image->setName(QString("Shock Filtered ") + current_image->name());
                    
                    m_phase_count = current_image->numBands();
                    
                    for( m_phase=0; m_phase < m_phase_count; m_phase++)
                    {	
                        shockFilter(current_image->band(m_phase),
                                    new_image->band(m_phase),
                                    param_iSigma->value(), param_oSigma->value(),
                                    param_upwind->value(), param_iterations->value());
                        
                        emit statusMessage(m_phase*99.0/m_phase_count, QString("filtering"));
                    }

                    QString descr("The following parameters were used for filtering:\n");
                    descr += m_parameters->valueText("ModelParameter");
                    new_image->setDescription(descr);
                                
                    delete m_results[0];
                    m_results[0] = new_image;
                    
                    emit statusMessage(100.0, QString("finished computation"));
                    emit finished();
                }
                catch(std::exception& e)
                {
                    emit errorMessage(QString("Explainable error occured: ") + QString::fromStdString(e.what()));
                }
                catch(...)
                {
                    emit errorMessage(QString("Non-explainable error occured"));		
                }
                unlockModels();
            }
        }
};

/** 
 * Creates one instance of the ShockFilter
 * algorithm defined above.
 *
 * \return A new instance of the ShockFilter.
 */
Algorithm* createShockFilter(Workspace * wsp)
{
	return new ShockFilter(wsp);
}




/**
 * This class encapsulates all the functionality of this module in a 
 * way that it can be used within graipe. To achieve this, it extends
 * graipe's Module class by introducing new algorithms.
 */
class ImageFilterModule
:   public Module
{
	public:
        /**
         * Default constructor for the ImageFilterModule.
         */
		ImageFilterModule()
		{
        }
    
        /**
         * Returns the ModelFactory of the ImageFilterModule.
         * Since this module does not introduce any new Models, it will
         * return an empty ModelFactory.
         *
         * \return An empty ModelFactory of the ImageFilterModule.
         */
		ModelFactory modelFactory() const
		{
			return ModelFactory();
		}
    
        /**
         * Returns the ViewControllerFactory of the ImageFilterModule.
         * Since this module does not introduce any new Models,  no views are
         * needed. Thus, it will return an empty ViewControllerFactory.
         *
         * \return An empty ViewControllerFactory of the ImageFilterModule.
         */
		ViewControllerFactory viewControllerFactory() const
		{
			return ViewControllerFactory();
		}
		
        /**
         * Returns the AlgorithmFactory of the ImageFilterModule.
         * Here, we provide the tree algorithms defined above.
         *
         * \return An AlgorithmFactory containing the analysis algorithms defined herein.
         */
		AlgorithmFactory algorithmFactory() const
		{
			AlgorithmFactory alg_factory;
            
			AlgorithmFactoryItem alg_item;
			alg_item.topic_name = "Image filters";
			
			//1. Frost Filter
			alg_item.algorithm_name = "Frost filter";
            alg_item.algorithm_type = "FrostFilter";
			alg_item.algorithm_fptr = &createFrostFilter;
			alg_factory.push_back(alg_item);
			
			alg_item.algorithm_name = "Enhanced Frost filter";
            alg_item.algorithm_type = "EnhancedFrostFilter";
			alg_item.algorithm_fptr = &createEnhancedFrostFilter;
			alg_factory.push_back(alg_item);
			
			//2. Gamma Filter
			alg_item.algorithm_name = "Gamma MAP filter";	
            alg_item.algorithm_type = "GammaMAPFilter";
			alg_item.algorithm_fptr = &createGammaMAPFilter;
			alg_factory.push_back(alg_item);
			
			//3. Kuan Filter
			alg_item.algorithm_name = "Kuan filter";	
            alg_item.algorithm_type = "KuanFilter";
			alg_item.algorithm_fptr = &createKuanFilter;
			alg_factory.push_back(alg_item);
			
			//4. Lee Filter
			alg_item.algorithm_name = "Lee filter";	
            alg_item.algorithm_type = "LeeFilter";
			alg_item.algorithm_fptr = &createLeeFilter;
			alg_factory.push_back(alg_item);
			
			alg_item.algorithm_name = "Enhanced Lee filter";
            alg_item.algorithm_type = "EnhancedLeeFilter";
			alg_item.algorithm_fptr = &createEnhancedLeeFilter;
			alg_factory.push_back(alg_item);
			
			//5. Median Filter
			alg_item.algorithm_name = "Median filter";		
            alg_item.algorithm_type = "MedianFilter";
			alg_item.algorithm_fptr = &createMedianFilter;
			alg_factory.push_back(alg_item);
			
			//6. Shock Filter
			alg_item.algorithm_name = "Coherence enhancing shock filter";	
            alg_item.algorithm_type = "ShockFilter";
			alg_item.algorithm_fptr = &createShockFilter;
			alg_factory.push_back(alg_item);
			
			return alg_factory;
		}
		
        /**
         * Returns the name of this Module.
         * 
         * \return Always: "ImageFilterModule"
         */
		QString name() const
        {
            return "ImageFilterModule";
        }
};

/**
 * @}
 */

} //end of namespace graipe


/**
 * @addtogroup graipe_imagefilters
 * @{
 */
#include <QtCore/QtGlobal>
extern "C"{
    /**
     *  The initialization procedure returns a pointer to the
     *  ImageFilterModule (which inherits from Module) acutal
     *  implementation of the class above
     *
     *	\return The pointer to a new instance of this module.
     */
    Q_DECL_EXPORT graipe::Module* initialize()
	{
        return new graipe::ImageFilterModule;
	}
} //extern "C"

/**
 * @}
 */
