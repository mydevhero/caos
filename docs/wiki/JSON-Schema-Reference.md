# 🔧 JSON Schema Reference

## Overview

CAOSDBA uses JSON Schema to validate query definition files. This ensures that all query definitions follow the correct structure and prevents common errors during build time.

## Schema Location

The JSON Schema is located at:
- schemas/query-schema.json (in the CAOSDBA repository)

## Schema consumer script

The script is located at:
- libcaos/generate_queries.py

## Schema Definition

### Full Schema

```json
{
  "$schema": "http://json-schema.org/draft/2020-12/schema",
  "$id": "https://github.com/mydevhero/CAOSDBA/schemas/query-schema.json",
  "title": "CAOSDBA Query Definition Schema",
  "description": "Schema for validating CAOSDBA query definition YAML files",
  "type": "object",
  "properties": {
    "queries": {
      "type": "array",
      "description": "List of query definitions",
      "items": {
        "type": "object",
        "properties": {
          "name": {
            "type": "string",
            "pattern": "^[A-Za-z_][A-Za-z0-9_]*$",
            "description": "C++ method name (must be valid identifier)"
          },
          "return_type": {
            "type": "string",
            "description": "C++ return type of the query"
          },
          "enabled": {
            "type": "boolean",
            "default": true,
            "description": "Master switch for query generation"
          },
          "metadata": {
            "type": "object",
            "description": "Query metadata and categorization",
            "properties": {
              "category": {
                "type": "string",
                "enum": ["standard", "example", "template"],
                "default": "standard",
                "description": "Query category for conditional generation"
              }
            },
            "default": {},
            "additionalProperties": false
          },
          "parameters": {
            "type": "array",
            "description": "Function parameters",
            "items": {
              "type": "object",
              "properties": {
                "type": {
                  "type": "string",
                  "description": "C++ type of the parameter"
                },
                "name": {
                  "type": "string",
                  "pattern": "^[a-z][a-zA-Z0-9_]*$",
                  "description": "Parameter name (must be valid C++ identifier)"
                },
                "default": {
                  "type": "string",
                  "description": "Default value as C++ expression"
                },
                "description": {
                  "type": "string",
                  "description": "Human-readable description"
                }
              },
              "required": ["type", "name"],
              "additionalProperties": false
            },
            "default": []
          },
          "authentication": {
            "type": "object",
            "description": "Authentication configuration",
            "properties": {
              "type": {
                "type": "string",
                "enum": ["TOKEN", "NONE"],
                "default": "NONE",
                "description": "Authentication type"
              },
              "env_var": {
                "type": "string",
                "description": "Environment variable name for the secret"
              },
              "required": {
                "type": "boolean",
                "default": true,
                "description": "Whether authentication is mandatory"
              }
            },
            "required": ["type"],
            "additionalProperties": false
          }
        },
        "required": ["name", "return_type"],
        "additionalProperties": false
      },
      "minItems": 0
    }
  },
  "required": ["queries"],
  "additionalProperties": false
}
```

## Field Validation Rules

### name Field

* Type: string
* Pattern: ^[A-Za-z_][A-Za-z0-9_]*$
* Description: Must be a valid C++ identifier
* Valid: getUser, IQuery_Example_test, process_data_v2
* Invalid: 123start, my-query, query.name

### metadata.category Field

* Type: string
* Enum: ["standard", "example", "template"]
* Default: "standard"
* Description: Controls CLI flag requirements
* Note: "example" and "template" are meant to be defined for CAOSDBA internal use

### parameters[].name Field

* Type: string
* Pattern: ^[a-z][a-zA-Z0-9_]*$
* Description: Must start with lowercase letter
* Valid: userId, max_count, include_profile
* Invalid: UserID, _private, 123param

### authentication.type Field

* Type: string
* Enum: ["TOKEN", "NONE"]
* Default: "NONE"
* Dependencies:
  - If type: "TOKEN", env_var is required
  - If type: "NONE", env_var must not be present

## Validation Examples

### Valid Query Definition

```yaml
queries:
  - name: getUserProfile
    return_type: std::optional<UserProfile>
    metadata:
      category: standard
    parameters:
      - type: int
        name: user_id
    authentication:
      type: TOKEN
      env_var: API_TOKEN
      required: true
```

## Invalid Examples

```yaml
queries:
  # ❌ Missing required field
  - name: invalidQuery
    # return_type missing - validation error
  
  # ❌ Invalid parameter name
  - name: anotherQuery
    return_type: void
    parameters:
      - type: int
        name: 123badname  # Starts with number
  
  # ❌ Missing env_var for TOKEN auth
  - name: authQuery
    return_type: bool
    authentication:
      type: TOKEN
      # env_var missing - validation error
  
  # ❌ Invalid category
  - name: categoryQuery
    return_type: string
    metadata:
      category: experimental  # Not in enum
```

## Schema Features

### Default Values

- enabled: defaults to true
- metadata.category: defaults to "standard"
- authentication.type: defaults to "NONE"
- authentication.required: defaults to true
- Empty metadata object is allowed
- Empty parameters array is allowed

### Strict Validation

- additionalProperties: false on all objects
- Prevents typos and unknown fields
- Ensures clean, predictable structure

### Pattern Validation

- Method names: C++ identifier pattern
- Parameter names: lowercase-start pattern
- Prevents common naming errors

## Using the Schema

### Automatic validation

The schema is automatically used when:

- running cmake configuration
- edit queries.yaml

## Manual Validation

You can validate YAML files manually using tools like:

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

## Troubleshooting Schema Errors

### Common Error Messages

1. 'some_field' is not allowed
  - You're using a field not defined in the schema
  - Check for typos or remove the field
1. 'some_field' is a required property
  - Missing a required field
  - Add the missing field
1. 'value' is not one of ['standard', 'example', 'template']
  - Invalid category value
  - Use one of the allowed values
1. '123name' does not match pattern
  - Invalid identifier pattern
  - Follow C++ naming convention

## Schema Versioning

The schema includes version information:

* $id: Contains version in URL
* Changes to the schema will update the version
* Backward compatibility maintained within major versions

### Best Practices
1. Always enable validation in production builds
1. Fix schema warnings during development
1. Use IDE integration for real-time validation
1. Validate before commits to catch errors early
1. Keep schemas versioned with your query definitions
