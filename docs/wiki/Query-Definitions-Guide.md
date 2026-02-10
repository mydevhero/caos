# 📖 Query Definitions Guide

## Overview

CAOSDBA uses YAML files with JSON Schema validation to define queries. This system replaces the legacy TXT format with a structured, validated approach that provides better organization, extensibility, and error prevention.

## YAML Format Structure

### Basic Structure

```yaml
queries:
  - name: echoString
    return_type: std::optional<std::string>
    enabled: true
    metadata:
      category: standard
    parameters:
      - type: std::string
        name: str
    authentication:
      type: TOKEN
      env_var: CAOS_API_TOKEN
      required: true
```
### Complete Example

```yaml
# CAOSDBA Query Definitions
version: "1.0.0"

queries:
  # Echo
  - name: IQuery_Template_echoString
    enabled: true
    metadata:
      category: template
    return_type: std::optional<std::string>
    description: "Echo a string back"

    parameters:
      - type: std::string
        name: str
        description: "Input string to echo"

    authentication:
      type: TOKEN
      required: true
      env_var: CAOS_API_TOKEN
      validation:
        min_length: 32
        max_length: 64

    tags: [template]
```

### Field Definitions

**name (required)**

- Type: string
- Description: The C++ method name that will be generated
- Pattern: Must be a valid C++ identifier
- Example: IQuery_Template_echoString

**return_type (required)**

- Type: string
- Description: C++ return type of the query
- Examples: C++ valid signature

**enabled (optional, default: true)**

* Type: boolean
* Description: Master switch for query generation
* Behavior:
  - true: Query will be considered for generation
  - false: Query will never be generated

**parameters (optional, default: [])**

* Type: array of objects
* Description: Function parameters in C++ format
* Fields per parameter:
  - type (required): C++ type (e.g., std::string, int, bool)
  - name (required): Parameter name
  - default (optional): Default value as C++ expression
  - description (optional): Human-readable description

**authentication (optional, default: type: NONE)**

* Type: object
* Description: Authentication configuration
* Fields:
  - type (required): TOKEN or NONE
  - env_var (required if type is TOKEN): Environment variable name
  - required (optional, default: true): Whether auth is mandatory

## Parameter Parsing Examples

### Simple Parameters

```yaml
parameters:
  - type: std::string
    name: username
  - type: int
    name: user_id
```

Generates: std::string username, int user_id

### With Default Values

```yaml
parameters:
  - type: bool
    name: verbose
    default: "false"
  - type: int
    name: limit
    default: "100"
```

### Complex Types

```yaml
parameters:
  - type: std::vector<std::string>
    name: filters
  - type: std::optional<int>
    name: timeout_ms
```

### Authentication Examples

**Token Authentication**
```yaml
authentication:
  type: TOKEN
  env_var: API_SECRET_TOKEN
  required: true
```

**No Authentication**
```yaml
authentication:
  type: NONE
# or simply omit the authentication field
```

## Best Practices

1. Use explicit categories for new queries instead of relying on naming patterns
1. Group related queries with comments in the YAML file
1. Keep enabled: true for active queries, disable deprecated ones
1. Use descriptive parameter names that match their purpose
1. Document complex parameters with the description field
1. Validate your YAML with JSON Schema before building

## Validation

### Automatic validation

CAOSDBA automatically validates your query definitions against a JSON Schema when:

- running cmake configuration
- edit queries.yaml

### Manual validation

```bash
# cd ${PROJECT_ROOT}

# Basic validation
python3 ./bin/validate_queries.py queries.yaml

# With custom schema
python3 ./bin/validate_queries.py queries.yaml --schema custom-schema.json

# Strict mode (exit with error on failure)
python3 ./bin/validate_queries.py queries.yaml --strict

# Verbose output
python3 ./bin/validate_queries.py queries.yaml --verbose

# Use default queries.yaml in current directory
python3 ./bin/validate_queries.py
```

## Troubleshooting

Common Errors

* "YAML root must be a dictionary"
  - Ensure your file starts with queries: at the top level
  - Check for missing indentation or incorrect YAML syntax
* "Invalid category"
  - Categories must be standard, example, or template
  - Check spelling and capitalization
* "Query disabled but CLI flag present"
  - Either enable the query (enabled: true)
  - Or don't pass the corresponding CLI flag
* "Missing required field"
  - All queries must have name and return_type
  - Parameters must have both type and name

