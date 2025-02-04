// close_popper.h 

#pragma once

namespace ClosePopper {
    enum class Type {
        Settings,
        Bookmarks,
        LineJump
    };
    
    void closeAllExcept(Type keepOpen);
    void closeAll();
}