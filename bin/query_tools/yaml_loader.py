#!/usr/bin/env python3
"""
YAML loader utilities for CAOSDBA query definitions.
Shared module for loading and basic validation of YAML files.
"""

import sys
from pathlib import Path
from typing import Any, List, Optional

import yaml


class YAMLLoaderError(Exception):
    """Base exception for YAML loading errors."""

    pass


def load_yaml_file(file_path: Path) -> Any:
    """
    Load YAML file, handling both single and multi-document formats.
    
    Args:
        file_path: Path to YAML file.
        
    Returns:
        Parsed YAML data (usually a dictionary).
        
    Raises:
        FileNotFoundError: If YAML file doesn't exist.
        YAMLLoaderError: If YAML file is invalid or empty.
    """
    if not file_path.exists():
        raise FileNotFoundError(f"YAML file not found: {file_path}")
    
    try:
        with open(file_path, 'r') as f:
            content = f.read()
        
        # Try to load as single document first
        data = yaml.safe_load(content)
        
        # If result is None (empty file) or not a dict, try loading all documents
        if data is None:
            # Try to load all documents to see if there are multiple
            all_docs = list(yaml.safe_load_all(content))
            if not all_docs:
                raise YAMLLoaderError("YAML file is empty")
            elif len(all_docs) > 1:
                raise YAMLLoaderError(
                    f"YAML file contains {len(all_docs)} documents. "
                    "Expected a single document with 'queries' root."
                )
            else:
                data = all_docs[0]
        
        return data
        
    except yaml.YAMLError as e:
        raise YAMLLoaderError(f"Invalid YAML format: {e}")


def load_yaml_string(yaml_content: str) -> Any:
    """
    Load YAML from string content.
    
    Args:
        yaml_content: YAML content as string.
        
    Returns:
        Parsed YAML data.
        
    Raises:
        YAMLLoaderError: If YAML content is invalid or empty.
    """
    try:
        # Try single document first
        data = yaml.safe_load(yaml_content)
        
        # Handle multi-document YAML
        if data is None:
            all_docs = list(yaml.safe_load_all(yaml_content))
            if not all_docs:
                raise YAMLLoaderError("YAML content is empty")
            elif len(all_docs) > 1:
                raise YAMLLoaderError(
                    f"YAML content contains {len(all_docs)} documents. "
                    "Expected a single document."
                )
            else:
                data = all_docs[0]
        
        if data is None:
            raise YAMLLoaderError("YAML content is empty")
        
        return data
        
    except yaml.YAMLError as e:
        raise YAMLLoaderError(f"Invalid YAML format: {e}")


def validate_yaml_structure(file_path: Path) -> None:
    """
    Diagnostic function to examine YAML structure.
    
    Args:
        file_path: Path to YAML file.
        
    Prints:
        Diagnostic information about the YAML structure.
    """
    print(f"🔍 Diagnosing YAML structure: {file_path}")
    print("─" * 60)
    
    try:
        with open(file_path, 'r') as f:
            content = f.read()
        
        # Show first few lines
        lines = content.split('\n')
        print("First 10 lines:")
        for i, line in enumerate(lines[:10]):
            print(f"  {i+1:3}: {line}")
        
        print("\n" + "─" * 60)
        
        # Load all documents
        documents = list(yaml.safe_load_all(content))
        print(f"Number of YAML documents found: {len(documents)}")
        
        for i, doc in enumerate(documents):
            print(f"\nDocument {i+1}:")
            if doc is None:
                print("  (empty document)")
            elif isinstance(doc, dict):
                print(f"  Type: dict with keys: {list(doc.keys())}")
                # Show structure of first level
                for key, value in doc.items():
                    if isinstance(value, list):
                        print(f"  - {key}: list with {len(value)} items")
                    else:
                        print(f"  - {key}: {type(value).__name__}")
            else:
                print(f"  Type: {type(doc).__name__}")
                print(f"  Content: {repr(doc)[:100]}...")
        
        # Check for document separators
        if '---' in content:
            print("\n⚠️  Found '---' document separators in YAML")
            print("   The schema expects a single document with 'queries' root.")
            print("   Remove '---' separators or combine documents.")
        
    except Exception as e:
        print(f"Error during diagnosis: {e}")

# vim: set tabstop=2 shiftwidth=2 expandtab colorcolumn=121 :
