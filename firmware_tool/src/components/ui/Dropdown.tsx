import React from 'react';
import { Check } from 'lucide-react';

interface DropdownOption {
  value: string;
  label: string;
  icon?: React.ReactNode;
  color?: string;
}

interface DropdownProps {
  options: DropdownOption[];
  value: string | string[];
  onChange: (value: string | string[]) => void;
  multiple?: boolean;
  label: string;
  icon?: React.ReactNode;
  disabled?: boolean;
  className?: string;
}

export function Dropdown({
  options,
  value,
  onChange,
  multiple = false,
  label,
  icon,
  disabled = false,
  className = '',
}: DropdownProps) {
  const [isOpen, setIsOpen] = React.useState(false);
  const dropdownRef = React.useRef<HTMLDivElement>(null);

  React.useEffect(() => {
    function handleClickOutside(event: MouseEvent) {
      if (dropdownRef.current && !dropdownRef.current.contains(event.target as Node)) {
        setIsOpen(false);
      }
    }
    document.addEventListener('mousedown', handleClickOutside);
    return () => document.removeEventListener('mousedown', handleClickOutside);
  }, []);

  const handleOptionClick = (optionValue: string) => {
    if (multiple) {
      const currentValues = Array.isArray(value) ? value : [];
      const newValues = currentValues.includes(optionValue)
        ? currentValues.length > 1
          ? currentValues.filter(v => v !== optionValue)
          : currentValues
        : [...currentValues, optionValue];
      onChange(newValues);
    } else {
      onChange(optionValue);
      setIsOpen(false);
    }
  };

  const isSelected = (optionValue: string) => {
    if (multiple) {
      return Array.isArray(value) && value.includes(optionValue);
    }
    return value === optionValue;
  };

  return (
    <div className={`relative ${className}`} ref={dropdownRef}>
      <button
        onClick={() => !disabled && setIsOpen(!isOpen)}
        disabled={disabled}
        className={`
          flex items-center gap-2 rounded-lg px-3 py-1.5 text-sm w-full
          transition-colors duration-200
          focus:outline-none focus:ring-2 focus:ring-blue-500
          ${disabled 
            ? 'bg-gray-800/30 text-gray-500 cursor-not-allowed'
            : 'bg-gray-900 text-gray-300 hover:bg-gray-800'
          }
        `}
      >
        {icon && <span className={`${disabled ? 'opacity-50' : 'text-gray-500'}`}>{icon}</span>}
        <span className="flex-1 text-left">{label}</span>
        <svg 
          className={`h-4 w-4 fill-current ${disabled ? 'text-gray-600' : 'text-gray-400'} transition-transform ${isOpen ? 'rotate-180' : ''}`} 
          viewBox="0 0 20 20"
        >
          <path 
            fillRule="evenodd" 
            d="M5.293 7.293a1 1 0 011.414 0L10 10.586l3.293-3.293a1 1 0 111.414 1.414l-4 4a1 1 0 01-1.414 0l-4-4a1 1 0 010-1.414z" 
            clipRule="evenodd" 
          />
        </svg>
      </button>
      
      {isOpen && (
        <div className="absolute right-0 mt-1 w-full bg-gray-900 rounded-lg shadow-lg py-1 z-10">
          {multiple && (
            <div className="px-3 py-2 text-xs font-medium text-gray-400 uppercase border-b border-gray-800">
              Select Options
            </div>
          )}
          <div className="px-2">
            {options.map((option) => (
              <label
                key={option.value}
                className="flex items-center gap-2 px-2 py-1.5 hover:bg-gray-800 rounded cursor-pointer"
              >
                {multiple ? (
                  <input
                    type="checkbox"
                    checked={isSelected(option.value)}
                    onChange={() => handleOptionClick(option.value)}
                    className={`rounded border-gray-600 ${
                      option.color || 'text-blue-500'
                    } focus:ring-blue-500 focus:ring-offset-gray-900`}
                  />
                ) : (
                  <span className="w-4 h-4 flex-shrink-0">
                    {isSelected(option.value) && (
                      <Check className="h-4 w-4 text-blue-500" />
                    )}
                  </span>
                )}
                {option.icon && <span>{option.icon}</span>}
                <span className="text-sm text-gray-300">{option.label}</span>
              </label>
            ))}
          </div>
        </div>
      )}
    </div>
  );
}