# **CacheX Protocol: Request and Response Format**

## **Request Format**
CacheX follows a **binary protocol** for efficient parsing and communication. Each request consists of **multiple arguments** encoded in a structured format.

### **Structure**
| Field  | Type   | Description |
|--------|--------|-------------|
| `nstr` | `uint32_t` | Number of arguments (e.g., `SET key value` has 3 arguments) |
| `len1` | `uint32_t` | Length of the first argument |
| `str1` | `char[]` | First argument (e.g., `"SET"`) |
| `len2` | `uint32_t` | Length of the second argument |
| `str2` | `char[]` | Second argument (e.g., `"key"`) |
| `lenN` | `uint32_t` | Length of the Nth argument |
| `strN` | `char[]` | Nth argument |

### **Binary Layout**
```
+------+-----+------+-----+------+-----+-----+------+
| nstr | len | str1 | len | str2 | ... | len | strN |
+------+-----+------+-----+------+-----+-----+------+
   4B     4B   ...
```

### **Example**
#### **SET Request**
**Command:** `SET key value`  
**Binary Format (Network Order)**
```
[ 0x00000003 ]  // Number of arguments (3)
[ 0x00000003 ]  // Length of "SET"
[ "SET" ]       // Command
[ 0x00000003 ]  // Length of "key"
[ "key" ]       // Key
[ 0x00000005 ]  // Length of "value"
[ "value" ]     // Value
```

#### **GET Request**
**Command:** `GET key`  
**Binary Format**
```
[ 0x00000002 ]  // Number of arguments (2)
[ 0x00000003 ]  // Length of "GET"
[ "GET" ]       // Command
[ 0x00000003 ]  // Length of "key"
[ "key" ]       // Key
```

---

## **Response Format**
The response contains a **status code** followed by optional **data**.

### **Structure**
| Field     | Type   | Description |
|-----------|--------|-------------|
| `length`  | `uint32_t` | Total response length (excluding itself) |
| `status`  | `uint32_t` | Response status code |
| `data`    | `char[]` | (Optional) Response data |

### **Binary Layout**
```
+--------+-----------+
| length | status    |
+--------+-----------+
|  data  |(optional) |
+--------------------+
   4B       ...
```

### **Response Status Codes**
| Code  | Meaning |
|-------|---------|
| `0` (`RES_OK`) | Success |
| `1` (`RES_ERR`) | Error |
| `2` (`RES_NX`) | Key not found |

---

### **Example Responses**
#### **SET Response**
```
[ 0x00000004 ]  // Response length (4 bytes for status)
[ 0x00000000 ]  // Status: RES_OK (0)
```

#### **GET Response (Key Found)**
```
[ 0x00000009 ]  // Response length (4 bytes for status + 5 bytes data)
[ 0x00000000 ]  // Status: RES_OK (0)
[ "value" ]     // Data: "value"
```

#### **GET Response (Key Not Found)**
```
[ 0x00000004 ]  // Response length (4 bytes for status)
[ 0x00000002 ]  // Status: RES_NX (2)
```

#### **Invalid Command Response**
```
[ 0x00000004 ]  // Response length (4 bytes for status)
[ 0x00000001 ]  // Status: RES_ERR (1)
```

---

Handles large requests with up to **200,000 arguments** safely.