h4_executable(h4-tool
  SOURCES main.cpp gog.cpp inspect.cpp render.cpp render_dialogs.cpp tool.hpp
  LINK h4::app h4::assets h4::graphics h4::io h4::model h4::rules h4::scene h4::screens h4::ui)
