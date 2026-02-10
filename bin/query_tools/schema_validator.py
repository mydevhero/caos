#!/usr/bin/env python3
"""
JSON Schema validator for CAOSDBA query definitions.
Shared module for validating query definitions against JSON Schema.
"""

import json
import sys
from pathlib import Path
from typing import Any, Dict, Optional, Tuple, List

import jsonschema
import jsonschema.exceptions

from .yaml_loader import YAMLLoaderError, load_yaml_file, load_yaml_string


class SchemaValidationError(Exception):
    """Raised when query definitions fail schema validation."""

    pass


class QueryValidator:
    """Validates query definitions against JSON Schema."""

    def __init__(self, schema_path: Optional[Path] = None):
        """
        Initialize validator with schema.

        Args:
            schema_path: Path to JSON Schema file. If None, uses default location.
        """
        if schema_path is None:
            # Default schema location relative to this module
            script_dir = Path(__file__).parent.parent.parent
            self.schema_path = script_dir / "schemas/CAOSDBA/queries.json"
        else:
            self.schema_path = Path(schema_path)

        self.schema = None
        self._load_schema()

    def _load_schema(self) -> None:
        """Load JSON Schema from file."""
        if not self.schema_path.exists():
            raise FileNotFoundError(f"Schema file not found: {self.schema_path}")

        try:
            with open(self.schema_path, 'r') as f:
                self.schema = json.load(f)
        except json.JSONDecodeError as e:
            raise ValueError(f"Invalid JSON in schema file: {e}")

    def validate_file(self, yaml_path: Path) -> bool:
        """
        Validate YAML file against schema.

        Args:
            yaml_path: Path to YAML query definitions file.

        Returns:
            True if validation passes, False otherwise.

        Raises:
            FileNotFoundError: If YAML file doesn't exist.
            YAMLLoaderError: If YAML file is invalid.
        """
        # Load YAML data using shared loader
        data = load_yaml_file(yaml_path)

        # Validate against schema
        return self._validate_with_detailed_errors(data, str(yaml_path))

    def validate_string(self, yaml_content: str, source_name: str = "string") -> bool:
        """
        Validate YAML string against schema.

        Args:
            yaml_content: YAML content as string.
            source_name: Name of data source for error messages.

        Returns:
            True if validation passes, False otherwise.
        """
        try:
            data = load_yaml_string(yaml_content)
            return self._validate_with_detailed_errors(data, source_name)
        except YAMLLoaderError as e:
            print(f"❌ Invalid YAML in {source_name}: {e}", file=sys.stderr)
            return False

    def validate_data(self, data: Dict, source_name: str = "input") -> bool:
        """
        Validate data dictionary against schema.

        Args:
            data: Dictionary containing query definitions.
            source_name: Name of data source for error messages.

        Returns:
            True if validation passes, False otherwise.
        """
        return self._validate_with_detailed_errors(data, source_name)

    def validate_multiple_files(self, files: List[Tuple[Path, str]]) -> Tuple[bool, List[str]]:
        """
        Validate multiple YAML files.

        Args:
            files: List of tuples (file_path, source_name)

        Returns:
            Tuple (all_valid, error_messages)
        """
        all_valid = True
        error_messages = []

        for file_path, source_name in files:
            try:
                if not self.validate_file(file_path):
                    all_valid = False
                    error_messages.append(f"Validation failed for {source_name}: {file_path}")
            except Exception as e:
                all_valid = False
                error_messages.append(f"Error validating {source_name}: {e}")

        return all_valid, error_messages

    def _validate_with_detailed_errors(self, data: Dict, source_name: str) -> bool:
        """
        Internal method to validate data with detailed error reporting.

        Args:
            data: Dictionary to validate.
            source_name: Name of data source for error messages.

        Returns:
            True if validation passes, False otherwise.
        """
        if self.schema is None:
            raise RuntimeError("Schema not loaded")

        try:
            jsonschema.validate(data, self.schema)
            return True
        except jsonschema.exceptions.ValidationError as e:
            self._print_validation_error(e, source_name)
            return False
        except jsonschema.exceptions.SchemaError as e:
            print(f"❌ Schema error: {e}", file=sys.stderr)
            return False

    def _print_validation_error(self, error: jsonschema.exceptions.ValidationError,
                               source_name: str) -> None:
        """
        Print detailed validation error information.

        Args:
            error: ValidationError exception.
            source_name: Name of data source.
        """
        print(f"❌ Validation failed for {source_name}:", file=sys.stderr)
        print(f"   Error: {error.message}", file=sys.stderr)

        # Provide context about where the error occurred
        if error.path:
            path_str = " -> ".join(str(p) for p in error.path)
            print(f"   Path: {path_str}", file=sys.stderr)

        if error.schema_path:
            schema_path_str = " -> ".join(str(p) for p in error.schema_path)
            print(f"   Schema constraint: {schema_path_str}", file=sys.stderr)

        # Show actual value if available
        if hasattr(error, 'instance') and error.instance is not None:
            value_repr = repr(error.instance)
            if len(value_repr) > 100:
                value_repr = value_repr[:97] + "..."
            print(f"   Invalid value: {value_repr}", file=sys.stderr)

# vim: set tabstop=2 shiftwidth=2 expandtab colorcolumn=121 :
