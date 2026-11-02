#include "../../includes/utils/utils.h"

#include <QColor>

namespace utils {
    namespace text {
        QTextCharFormat GetColoredDate() {
            QTextCharFormat format;
            format.setForeground(QColor("#939daf"));
            format.setFontUnderline(true);
            return format;
        }

        QTextCharFormat GetColoredAbyss() {
            QTextCharFormat format;
            format.setForeground(QColor("#64cfff"));
            return format;
        }
    }
}
