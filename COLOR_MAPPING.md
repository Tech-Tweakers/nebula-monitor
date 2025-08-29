# 🎨 ESP32 TFT Color Mapping Documentation

## 📱 Hardware Configuration
- **Board**: Cheap Yellow Board (CYB)
- **Display**: ST7789 Driver
- **Library**: TFT_eSPI + LVGL 8.3.11
- **Color Inversion**: ENABLED (`TFT_INVERSION_ON`)

## 🔍 Color Inversion Discovery Process

### 🧪 Test Results
| **Code** | **Expected Color** | **Actual Display** | **Inversion Pattern** |
|----------|-------------------|-------------------|----------------------|
| `0xFF0000` | 🔴 RED | **🔷 CYAN** | `0x00FFFF` |
| `0x00FF00` | 🟢 GREEN | **🩷 PINK** | `0xFF00FF` |
| `0x0000FF` | 🔵 BLUE | **🟡 YELLOW** | `0xFFFF00` |
| `0xFFFF00` | 🟡 YELLOW | **🔵 BLUE** | `0x0000FF` |
| `0xFF00FF` | 🟣 MAGENTA | **🟢 LIGHT GREEN** | `0x00FF00` |
| `0x00FFFF` | 🔷 CYAN | **🔴 RED** | `0xFF0000` |

## 🎯 Color Inversion Formula

### 📐 Mathematical Approach
```
Inverted Color = 0xFFFFFF - Original Color
```

### 🔧 Practical Examples
- **To get BLUE**: Use `0x0000FF` → displays as **YELLOW**
- **To get YELLOW**: Use `0xFFFF00` → displays as **BLUE**
- **To get RED**: Use `0xFF0000` → displays as **CYAN**

## 🚀 Working Color Combinations

### ✅ Successfully Implemented Colors

#### **Sky Blue (Azul Claro)**
- **Desired**: `0x87CEEB` (Sky Blue)
- **Code Used**: `0x783114` (Dark Brown)
- **Result**: ✅ **AZUL CLARO** displayed correctly

#### **Black Background**
- **Desired**: `0x000000` (Black)
- **Code Used**: `0x000000` (Black)
- **Result**: ✅ **WHITE** displayed (inverted)

#### **Light Gray Items**
- **Desired**: `0xEEEEEE` (Light Gray)
- **Code Used**: `0x111111` (Very Dark Gray)
- **Result**: ✅ **LIGHT GRAY** displayed (inverted)

## 🎨 Color Palette for Future Use

### 🌈 Primary Colors (Inverted)
| **Desired Color** | **Code to Use** | **Notes** |
|-------------------|-----------------|-----------|
| 🔴 **RED** | `0x00FFFF` | Use CYAN code |
| 🟢 **GREEN** | `0xFF00FF` | Use MAGENTA code |
| 🔵 **BLUE** | `0xFFFF00` | Use YELLOW code |
| 🟡 **YELLOW** | `0x0000FF` | Use BLUE code |
| 🟣 **MAGENTA** | `0x00FF00` | Use GREEN code |
| 🔷 **CYAN** | `0xFF0000` | Use RED code |

### 🎭 UI Theme Colors
| **Element** | **Desired Color** | **Code to Use** | **Result** |
|-------------|-------------------|-----------------|------------|
| **Background** | White | `0x000000` | ✅ White |
| **Title Bar** | Dark Gray | `0x444444` | ✅ Dark Gray |
| **Buttons** | Sky Blue | `0x783114` | ✅ Sky Blue |
| **List Items** | Light Gray | `0x111111` | ✅ Light Gray |
| **Text** | White | `0xFFFFFF` | ✅ White |
| **Text (Secondary)** | Light Gray | `0xCCCCCC` | ✅ Light Gray |

## 🔧 Technical Implementation

### 📝 Code Example
```cpp
// To get SKY BLUE buttons:
uint32_t button_colors[] = {
    0x783114,  // Will display as SKY BLUE
    0x783114,  // Will display as SKY BLUE
    0x783114,  // Will display as SKY BLUE
    0x783114,  // Will display as SKY BLUE
    0x783114,  // Will display as SKY BLUE
    0x783114   // Will display as SKY BLUE
};

// To get BLACK background:
lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x000000), LV_PART_MAIN);
// Will display as WHITE due to inversion

// To get LIGHT GRAY list items:
lv_obj_set_style_bg_color(status_item, lv_color_hex(0x111111), LV_PART_MAIN);
// Will display as LIGHT GRAY due to inversion
```

## 🚨 Important Notes

### ⚠️ Color Inversion Behavior
- **ALL colors are inverted** on this display
- **No exceptions** - affects every pixel
- **Must account for inversion** in all UI designs

### 💡 Design Tips
1. **Always test colors** before finalizing
2. **Use the inverted color codes** for desired results
3. **Keep a reference** of working color combinations
4. **Consider contrast** - inverted colors may affect readability

## 📋 Quick Reference

### 🎯 Most Used Colors
- **Sky Blue**: `0x783114`
- **White**: `0x000000`
- **Light Gray**: `0x111111`
- **Dark Gray**: `0x444444`

### 🔍 Testing New Colors
1. **Choose desired color** (e.g., Purple)
2. **Find its inverse**: `0xFFFFFF - Purple`
3. **Use inverse code** in your design
4. **Test and verify** the result

---

## 📅 Document Version
- **Created**: Current Session
- **Last Updated**: Current Session
- **Status**: ✅ **ACTIVE & TESTED**

## 🎉 Success Story
**"Nebula Monitor v2.0"** - Complete interface with:
- ✅ Professional title
- ✅ Uniform sky blue buttons
- ✅ Clean white background
- ✅ Organized network status list
- ✅ Perfect color mapping implementation

---
*This document serves as the definitive reference for color management on the ESP32 TFT display with color inversion enabled.*
