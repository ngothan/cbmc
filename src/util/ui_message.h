/*******************************************************************\

Module:

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#ifndef CPROVER_UI_MESSAGE_H
#define CPROVER_UI_MESSAGE_H

#include "message.h"

class ui_message_handlert:public message_handlert
{
public:
  typedef enum { PLAIN, XML_UI, JSON_UI } uit;
  
  ui_message_handlert(uit __ui, const std::string &program);   
  explicit ui_message_handlert(const std::string &program);   

  virtual ~ui_message_handlert();

  inline uit get_ui() const
  {
    return _ui;
  }

  inline void set_ui(uit __ui)
  {
    _ui=__ui;
  }

protected:
  uit _ui;
 
  // overloading
  virtual void print(
    unsigned level,
    const std::string &message);

  // overloading
  virtual void print(
    unsigned level,
    const std::string &message,
    int sequence_number,
    const source_locationt &location);

  virtual void xml_ui_msg(
    const std::string &type,
    const std::string &msg1,
    const std::string &msg2,
    const source_locationt &location);

  virtual void json_ui_msg(
    const std::string &type,
    const std::string &msg1,
    const std::string &msg2,
    const source_locationt &location);

  virtual void ui_msg(
    const std::string &type,
    const std::string &msg1,
    const std::string &msg2,
    const source_locationt &location);

  const char *level_string(unsigned level);
};

#endif
