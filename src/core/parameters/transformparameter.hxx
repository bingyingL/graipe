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

#ifndef GRAIPE_CORE_PARAMETERS_TRANSFORMPARAMETER_HXX
#define GRAIPE_CORE_PARAMETERS_TRANSFORMPARAMETER_HXX

#include "core/parameters/parameter.hxx"

#include <QPointer>
#include <QLineEdit>

namespace graipe {

/**
 * @addtogroup graipe_core
 * @{
 *
 * @file
 * @brief Header file for the TransformParameter class
 */

/**
 * This is the TransformParameter class.
 * It inherits from the Parameter base class to:
 * - hold a QTransform value, and
 * - provide editing facilities by means of 3x3 QLineEdits.
 */
class GRAIPE_CORE_EXPORT TransformParameter
:   public Parameter
{
    Q_OBJECT
    
    public:
        /**
         * Default constructor of the TransformParameter class with a setting of the
         * most important values directly.
         *
         * \param name          The name (label) of this parameter.
         * \param value         The initial value of this parameter.
         * \param parent        If given (!= NULL), this parameter has a parent and will
         *                      be enabled/disabled, if the parent is a BoolParameter.
         * \param invert_parent If true, the enables/disabled dependency to the parent will be swapped.
         */
        TransformParameter(const QString& name, QTransform value=QTransform(), Parameter* parent=NULL, bool invert_parent=false);
    
        /**
         * Destructor of the TransformParameter class.
         */
        ~TransformParameter();
    
        /**
         * The (immutable) type name of this parameter class.
         *
         * \return "TransformParameter".
         */
        QString typeName() const
        {
            return "TransformParameter";
        }
        
        /** 
         * The current value of this parameter in the correct, most special type.
         *
         * \return The value of this parameter.
         */
        QTransform value() const;
    
        /**
         * Writing accessor of the current value of this parameter.
         *
         * \param value The new value of this parameter.
         */
        void setValue(const QTransform& value);
    
        /**
         * Serialization of the parameter's state to an output device.
         * Writes the following XML on the device:
         * 
         * \verbatim
           <TYPENAME>
               <Name>NAME</Name>
               <Transform Type="Affine">
                 <m11>value().m11()</m11>
                 <m12>value().m12()</m12>
                 <m13>value().m13()</m13>
                 <m21>value().m21()</m21>
                 <m22>value().m22()</m22>
                 <m23>value().m23()</m23>
                 <m31>value().m31()</m31>
                 <m32>value().m32()</m32>
                 <m33>value().m33()</m33>
               </Transform>
           </TYPENAME>
           \endverbatim
         *
         * with TYPENAME = typeName(),
         *         NAME = name().
         *
         * \param xmlWriter The QXmlStreamWriter on which we serialize the parameter's state.
         */
        void serialize(QXmlStreamWriter& xmlWriter) const;
    
        /**
         * Deserialization of a parameter's state from an xml file.
         *
         * \param xmlReader The QXmlStreamReader, where we read from.
         * \return True, if the deserialization was successful, else false.
         */
        bool deserialize(QXmlStreamReader& xmlReader);
    
        /**
         * This function indicates whether the value of a parameter is valid or not.
         *
         * \return True, if the parameter's value is valid.
         */
        bool isValid() const;
        
        /**
         * The delegate widget of this parameter. 
         * Each parameter generates such a widget on demand, which refers to the
         * first call of this function. This is needed due to the executability of
         * classes using parameters (like the Algorithm class) in different threads.
         *
         * \return The delegate widget to control the values of this parameter.
         */
        QWidget * delegate();
        
    public slots:    
        /**
         * This slot is called everytime, the delegate has changed. It has to synchronize
         * the internal value of the parameter with the current delegate's value
         */
        void updateValue();
        
    protected:
        /** The value of this parameter **/
        QTransform m_value;
    
        /** parent delegate **/
        QPointer<QWidget> m_delegate;
    
        /**
         * @{
         * the child delegates (3x3) for matrix **/
        QPointer<QLineEdit> m_lne11, m_lne12, m_lne13,
                            m_lne21, m_lne22, m_lne23,
                            m_lne31, m_lne32, m_lne33;
        /**
         * @}
         */
};

/**
 * @}
 */

} //end of namespace graipe

#endif //GRAIPE_CORE_PARAMETER_HXX
