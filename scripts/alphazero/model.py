import torch
import torch.nn as nn
import torch.nn.functional as F
import numpy as np


class PandemicNet(nn.Module):
    def __init__(self, input_size=682):
        super(PandemicNet, self).__init__()

        self.fc1 = nn.Linear(input_size, 512)
        self.fc2 = nn.Linear(512, 512)
        self.fc3 = nn.Linear(512, 256)
        self.value_head = nn.Linear(256, 1)

    def forward(self, x):
        if not torch.is_tensor(x):
            x = torch.FloatTensor(x)
        x = F.relu(self.fc1(x))
        x = F.relu(self.fc2(x))
        x = F.relu(self.fc3(x))
        return torch.tanh(self.value_head(x))

    def predict(self, state_tensor):
        """Convenience helper for inference."""
        self.eval()
        with torch.no_grad():
            t = torch.FloatTensor(state_tensor).unsqueeze(0)
            return self.forward(t).item()