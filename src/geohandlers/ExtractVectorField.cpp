/*---------------------------------------------------------------------------*\
 *
 *  mimmo
 *
 *  Copyright (C) 2015-2017 OPTIMAD engineering Srl
 *
 *  -------------------------------------------------------------------------
 *  License
 *  This file is part of mimmo.
 *
 *  mimmo is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License v3 (LGPL)
 *  as published by the Free Software Foundation.
 *
 *  mimmo is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with mimmo. If not, see <http://www.gnu.org/licenses/>.
 *
\*---------------------------------------------------------------------------*/

#include "ExtractFields.hpp"
#include "SkdTreeUtils.hpp"

namespace mimmo{

/*!
 * Default constructor.
 */
ExtractVectorField::ExtractVectorField():ExtractField(){
    m_name = "mimmo.ExtractVectorField";
}

/*!
 * Custom constructor reading xml data
 * \param[in] rootXML reference to your xml tree section
 */
ExtractVectorField::ExtractVectorField(const bitpit::Config::Section & rootXML){

    std::string fallback_name = "ClassNONE";
    std::string input_name = rootXML.get("ClassName", fallback_name);
    input_name = bitpit::utils::string::trim(input_name);

    m_name = "mimmo.ExtractVectorField";

    if(input_name == "mimmo.ExtractVectorField"){
        absorbSectionXML(rootXML);
    }else{
        warningXML(m_log, m_name);
    };
}

/*!
 * Default destructor
 */
ExtractVectorField::~ExtractVectorField(){}

/*!
 * Copy Constructor.
 * \param[in] other class of type ExtractField
 */
ExtractVectorField::ExtractVectorField(const ExtractVectorField & other):ExtractField(other){
	m_field = other.m_field;
	m_result = other.m_result;
}

/*!
 * Assignement operator of ExtractField.
 * \param[in] other class of type ExtractField
 */
ExtractVectorField & ExtractVectorField::operator=(const ExtractVectorField & other){
	ExtractField::operator=(other);
	m_field = other.m_field;
	m_result = other.m_result;
	return *this;
};

/*!
 * Swap function of ExtractVectorField
 * \param[in] x object to be swapped.
 */
void ExtractVectorField::swap(ExtractVectorField & x ) noexcept
{
//     std::swap(m_field,x.m_field);
//     std::swap(m_result, x.m_result);
    m_field.swap(x.m_field);
    m_result.swap(x.m_result);
    ExtractField::swap(x);
};


/*!
 * Build the ports of the class;
 */
void
ExtractVectorField::buildPorts(){

    ExtractField::buildPorts();

    bool built = m_arePortsBuilt;
    built = (built && createPortIn<dmpvecarr3E*, ExtractVectorField>(this, &mimmo::ExtractVectorField::setField, M_VECTORFIELD, true, 1));
    built = (built && createPortOut<dmpvecarr3E*, ExtractVectorField>(this, &mimmo::ExtractVectorField::getExtractedField, M_VECTORFIELD));

    m_arePortsBuilt = built;
}

/*!
 * Get extracted field.
 * \return extracted field
 */
dmpvecarr3E*
ExtractVectorField::getExtractedField(){
    return &m_result;
}

/*!
 * Get linked field where result need to be extracted from.
 * \return original field
 */
dmpvecarr3E
ExtractVectorField::getOriginalField(){
    return m_field;
}

/*!
 * Set input field for extraction.
 * \param[in] field input field related to whole geometry
 */
void
ExtractVectorField::setField(dmpvecarr3E *field){
    if(!field)  return;
    m_field = *field;
}

/*!
 * Clear content of the class
 */
void
ExtractVectorField::clear(){
    m_field.clear();
    m_result.clear();
    ExtractField::clear();
}

/*!
 * Plot extracted field over the target geometry
 */
void
ExtractVectorField::plotOptionalResults(){

    if (m_result.size() == 0 || m_result.getGeometry() == NULL) return;

    bitpit::VTKLocation loc = bitpit::VTKLocation::UNDEFINED;
    switch(m_result.getDataLocation()){
        case MPVLocation::POINT :
            loc = bitpit::VTKLocation::POINT;
            break;
        case MPVLocation::CELL :
            loc = bitpit::VTKLocation::CELL;
            break;
        default:
            (*m_log)<<"Error: Undefined Reference Location in plotOptionalResults of "<<m_name<<std::endl;
            (*m_log)<<"Interface or Undefined locations are not supported in VTU writing." <<std::endl;
            break;
    }
    if(loc == bitpit::VTKLocation::UNDEFINED){
        throw std::runtime_error("Undefined data location");
    };

    //check size of field and adjust missing values to zero for writing purposes only.
    dmpvecarr3E field_supp = m_result;
    if(!field_supp.completeMissingData({{0.0,0.0,0.0}})) return;
    dvecarr3E field = field_supp.getDataAsVector();


    if(m_result.getGeometry()->getType() != 3){
        m_result.getGeometry()->getPatch()->getVTK().setDirectory(m_outputPlot+"/");
        m_result.getGeometry()->getPatch()->getVTK().setName(m_name+std::to_string(getId()));
        m_result.getGeometry()->getPatch()->getVTK().addData("field", bitpit::VTKFieldType::VECTOR, loc, field);
        m_result.getGeometry()->getPatch()->write();
        m_result.getGeometry()->getPatch()->getVTK().removeData("field");
    }else{
        if(loc == bitpit::VTKLocation::CELL){
            (*m_log)<<"Error: Attempt writing Cell data field on cloud of points in plotOptionalResults of "<<m_name<<std::endl;
            throw std::runtime_error("Attempt writing Cell data field on cloud of points");
        }
        liimap mapDataInv;
        dvecarr3E points = m_result.getGeometry()->getVerticesCoords(&mapDataInv);
        int size = points.size();
        ivector2D connectivity(size, ivector1D(1));
        for(int i=0; i<size; ++i)    connectivity[i][0]=i;
        bitpit::VTKUnstructuredGrid output(m_outputPlot,m_name+std::to_string(getId()),bitpit::VTKElementType::VERTEX);
        output.setGeomData( bitpit::VTKUnstructuredField::POINTS, points);
        output.setGeomData( bitpit::VTKUnstructuredField::CONNECTIVITY, connectivity);
        output.setDimensions(connectivity.size(), points.size());
        output.addData("field", bitpit::VTKFieldType::VECTOR, loc, field);
        output.setCodex(bitpit::VTKFormat::APPENDED);
        output.write();
    }
}


/*!
 * Extract field from your original field by using the provided target geometry
 * \return true if extract without errors
 */
bool
ExtractVectorField::extract(){

    if (getGeometry() == NULL || m_field.getGeometry() == NULL) return false;
    //checking internal ids coherence of the field.
    if(!m_field.checkDataIdsCoherence()) return false;

    mimmo::MPVLocation refLoc = m_field.getDataLocation();
    if(refLoc == mimmo::MPVLocation::UNDEFINED) refLoc = mimmo::MPVLocation::POINT;

    m_result.clear();
    m_result.setDataLocation(refLoc);

    switch(m_mode){
        case ExtractMode::ID :
            extractID(refLoc);
            m_result.setGeometry(getGeometry());
            break;

        case ExtractMode::PID :
            extractPID(refLoc);
            m_result.setGeometry(m_field.getGeometry());
            break;

        case ExtractMode::MAPPING :
            extractMapping(refLoc);
            m_result.setGeometry(getGeometry());
            break;

        default :
            assert(false && "unrecognized field location");
            break;
    }

    if (m_result.isEmpty()) return false;

    return true;

}

/*!
 * Perform extraction by ID mode and refer data to target geometry vertex, cell or interface,
 * according to the location specified.
 * \param[in] loc MPVLocation enum identifying data location POINT, CELL or INTERFACE.
 */
void ExtractVectorField::extractID(mimmo::MPVLocation loc){

    switch(loc){
        case mimmo::MPVLocation::POINT:
            for (const auto & ID : getGeometry()->getVertices().getIds()){
                if (m_field.exists(ID)){
                    m_result.insert(ID, m_field[ID]);
                }
            }
            break;
        case mimmo::MPVLocation::CELL:
            for (const auto & ID : getGeometry()->getCells().getIds()){
                if (m_field.exists(ID)){
                    m_result.insert(ID, m_field[ID]);
                }
            }
            break;
        case mimmo::MPVLocation::INTERFACE:
            if(!getGeometry()->areInterfacesBuilt()) getGeometry()->buildInterfaces();
            for (const auto & ID : getGeometry()->getInterfaces().getIds()){
                if (m_field.exists(ID)){
                    m_result.insert(ID, m_field[ID]);
                }
            }
            break;
        default:
            break;
    }
}

/*!
 * Perform extraction by PID mode and refer data to target geometry vertex, cell or interface,
 * according to the location specified.
 * \param[in] loc MPVLocation enum identifying data location POINT, CELL or INTERFACE.
 */
void ExtractVectorField::extractPID(mimmo::MPVLocation loc){
    //Extract by PIDs
    livector1D commonPID;
    {
        std::unordered_set<long> pidTarget = getGeometry()->getPIDTypeList();
        std::unordered_set<long> pidLinked = m_field.getGeometry()->getPIDTypeList();
        std::set<long> unionPID(pidTarget.begin(), pidTarget.end());
        unionPID.insert(pidLinked.begin(), pidLinked.end());
        for(auto val: unionPID){
            if(pidLinked.count(val) && pidTarget.count(val)){
                commonPID.push_back(val);
            }
        }
    }
    livector1D cellExtracted = m_field.getGeometry()->extractPIDCells(commonPID);

    switch(loc){
        case mimmo::MPVLocation::POINT:
        {
            livector1D vertExtracted = m_field.getGeometry()->getVertexFromCellList(cellExtracted);
            for (const auto & ID : vertExtracted){
                if (m_field.exists(ID)){
                    m_result.insert(ID, m_field[ID]);
                }
            }
        }
        break;
        case mimmo::MPVLocation::CELL:
            for (const auto & ID : cellExtracted){
                if (m_field.exists(ID)){
                    m_result.insert(ID, m_field[ID]);
                }
            }
            break;
        case mimmo::MPVLocation::INTERFACE:
        {
            livector1D interfExtracted = m_field.getGeometry()->getInterfaceFromCellList(cellExtracted);
            for (const auto & ID : interfExtracted){
                if (m_field.exists(ID)){
                    m_result.insert(ID, m_field[ID]);
                }
            }
        }
        break;
        default:
            break;
    }
}

/*!
 * Perform extraction by MAPPING mode and refer data to target geometry vertex, cell or interface,
 * according to the location specified.
 * \param[in] loc MPVLocation enum identifying data location POINT, CELL or INTERFACE.
 */
void ExtractVectorField::extractMapping(mimmo::MPVLocation loc){

    if(! m_field.getGeometry()->isSkdTreeSync())  m_field.getGeometry()->buildSkdTree();
    if(! getGeometry()->isSkdTreeSync())          getGeometry()->buildSkdTree();
    livector1D cellExtracted = mimmo::skdTreeUtils::selectByPatch(m_field.getGeometry()->getSkdTree(), getGeometry()->getSkdTree(), m_tol);

    switch(loc){
        case mimmo::MPVLocation::POINT:
        {
            livector1D vertExtracted = getGeometry()->getVertexFromCellList(cellExtracted);
            for (const auto & ID : vertExtracted){
                if (m_field.exists(ID)){
                    m_result.insert(ID, m_field[ID]);
                }
            }
        }
        break;
        case mimmo::MPVLocation::CELL:
            for (const auto & ID : cellExtracted){
                if (m_field.exists(ID)){
                    m_result.insert(ID, m_field[ID]);
                }
            }
            break;
        case mimmo::MPVLocation::INTERFACE:
        {
            livector1D interfExtracted = getGeometry()->getInterfaceFromCellList(cellExtracted);
            for (const auto & ID : interfExtracted){
                if (m_field.exists(ID)){
                    m_result.insert(ID, m_field[ID]);
                }
            }
        }
        break;
        default:
            break;
    }
}


}
