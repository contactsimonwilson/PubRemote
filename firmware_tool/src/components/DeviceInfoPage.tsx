import React from "react";
import { FloatAccessoriesSelector } from "./FloatAccessoriesSelector";

const DeviceInfoPage: React.FC<unknown> = () => {
  return (
    <div className="rounded-lg bg-gray-900 p-6">
      <FloatAccessoriesSelector />
    </div>
  );
};

export default DeviceInfoPage;
