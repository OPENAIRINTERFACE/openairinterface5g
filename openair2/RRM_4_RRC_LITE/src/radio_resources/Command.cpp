#include <iostream>
#include <sstream>
//----------------------------------------------------------------------------
#include "Command.h"
#include "Exceptions.h"
//----------------------------------------------------------------------------
Command::Command(unsigned char equipment_typeP, unsigned char equipment_idP, unsigned char actionP, unsigned char object_typeP, unsigned char object_idP) :
        m_equipment_type(equipment_typeP),m_equipment_id(equipment_idP),m_action(actionP), m_object_type(object_typeP), m_object_id(object_idP)
//----------------------------------------------------------------------------
{
    Storage();
    m_is_data_written = false;
    this->writeChar(m_equipment_type);
    this->writeChar(m_equipment_id);
    this->writeChar(m_action);
    this->writeChar(m_object_type);
    this->writeChar(m_object_id);
};
//----------------------------------------------------------------------------
Command::Command(tcpip::Storage& storageP, size_t* sizeP)
//----------------------------------------------------------------------------
{
    Storage();
    m_is_data_written = false;
    if (*sizeP >= 7) {
        m_equipment_type = storageP.readChar();
        m_equipment_id   = storageP.readChar();
        m_action         = storageP.readChar();
        m_object_type    = storageP.readChar();
        m_object_id      = storageP.readChar();

        this->writeChar(m_equipment_type);
        this->writeChar(m_equipment_id);
        this->writeChar(m_action);
        this->writeChar(m_object_type);
        this->writeChar(m_object_id);

        unsigned short size = storageP.readShort();
        this->writeShort(size);
        *sizeP = *sizeP - 7;
        if (size > 0) {
            if (*sizeP >= size) {
                for (int i = 0; i < size ; i++) {
                    this->writeChar(storageP.readChar());
                }
            } else {
                throw command_deserialize_length_error();
            }
            *sizeP = *sizeP - size;
        }
        m_is_data_written = true;
    } else {
        throw command_deserialize_length_error();
    }
}
//----------------------------------------------------------------------------
void Command::AddData(tcpip::Storage& storageP)
//----------------------------------------------------------------------------
{
    if (!m_is_data_written) {
        m_is_data_written = true;
        //this->writeShort(storageP.size()); already done in storageP
        this->writeStorage(storageP);
    } else {
        throw command_data_overwrite_error();
    }
}
//----------------------------------------------------------------------------
unsigned int Command::size() const
//----------------------------------------------------------------------------
{
    if (m_is_data_written) {
        return Storage::size();
    } else {
        return Storage::size() + 2;
    }
}

//----------------------------------------------------------------------------
void Command::Serialize(unsigned char* packetP)
//----------------------------------------------------------------------------
{
    //std::cout <<  "[Command] Serialize()" << ToString() << std::endl;
    if (!m_is_data_written) {
        this->writeShort(0);
    }
    readPacket(packetP);
}
//----------------------------------------------------------------------------
std::string Command::ToString()
//----------------------------------------------------------------------------
{
    stringstream result;
    result << EquipmentType2String() << " ID: " << (unsigned int)m_equipment_id << " " << Action2String() << " " << ObjectType2String() << " ID: " << (unsigned int)m_object_id << "  CONFIG SIZE: " << (size() - 7);
    return result.str();
}
//----------------------------------------------------------------------------
std::string Command::EquipmentType2String() {
//----------------------------------------------------------------------------
    switch (m_equipment_type) {
        case COMMAND_EQUIPMENT_RADIO_GATEWAY: {
            return "COMMAND_EQUIPMENT_RADIO_GATEWAY" ;
            break;
        }
        case COMMAND_EQUIPMENT_MOBILE_TERMINAL: {
            return  "COMMAND_EQUIPMENT_MOBILE_TERMINAL";
            break;
        }
        default: {
            return "COMMAND_EQUIPMENT_UNKNOWN";
            break;
        }
    }
}
//----------------------------------------------------------------------------
std::string Command::Action2String() {
//----------------------------------------------------------------------------
    switch (m_action) {
        case COMMAND_ACTION_NULL: {
            return "ACTION_NULL" ;
            break;
        }
        case COMMAND_ACTION_ADD: {
            return  "ACTION_ADD";
            break;
        }
        case COMMAND_ACTION_REMOVE: {
            return  "ACTION_REMOVE";
            break;
        }
        case COMMAND_ACTION_MODIFY: {
            return  "ACTION_MODIFY";
            break;
        }
        default: {
            return "ACTION_UNKNOWN";
            break;
        }
    }
}
//----------------------------------------------------------------------------
std::string Command::ObjectType2String() {
//----------------------------------------------------------------------------
    switch (m_object_type) {
        case COMMAND_OBJECT_SIGNALLING_RADIO_BEARER: {
            return "OBJECT_SIGNALLING_RADIO_BEARER" ;
            break;
        }
        case COMMAND_OBJECT_DATA_RADIO_BEARER: {
            return  "OBJECT_DATA_RADIO_BEARER";
            break;
        }
        case COMMAND_OBJECT_MOBILE: {
            return  "OBJECT_MOBILE";
            break;
        }
        default: {
            return "OBJECT_UNKNOWN";
            break;
        }
    }
}
